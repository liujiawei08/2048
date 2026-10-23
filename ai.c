#include <stddef.h>

#include "ai.h"
#include "game_logic.h" /* 复用 BOARD_SIZE、Direction 与 slide_left() */

/* ===================== 可调参数 ===================== */

/* 迭代加深的最大深度（单位：玩家的一步移动）。
 * 深度越深看得越远，但节点数指数增长；配合节点预算保证低配置机器也瞬间响应。
 */
#define AI_MAX_DEPTH 4

/* 单层深度搜索的节点预算。每个节点只是几次循环，
 * 3 万个节点在现代 CPU 上不足 10 毫秒，十年前的机器上也不到 0.1 秒；
 * 迭代加深最多 4 层，整体决策稳定在百毫秒内。
 */
#define AI_NODE_BUDGET 30000L

/* 启发式评估权重（经验值，来自 2048 AI 社区的常用配置，可按需微调） */
#define W_EMPTY   270.0  /* 空格越多越好：空格=腾挪余地 */
#define W_SMOOTH  25.0   /* 相邻方块数值越接近越好：容易合并 */
#define W_MONO    60.0   /* 行/列尽量单调：大数压角落不散乱 */
#define W_CORNER  2.0    /* 最大方块位于角落时额外奖励（按 log2 值加权） */

/* 浮点“负无穷/正无穷”，用于 Alpha-Beta 初始界 */
#define NEG_INF (-1.0e18)
#define POS_INF ( 1.0e18)

/* 搜索期间累计访问的节点数，超过预算立即退化为启发式评估 */
static long nodes_used;

/* ===================== 小工具 ===================== */

/* 整数 log2（只用于 2 的幂；0 不会出现，防御性返回 0） */
static int ilog2(int v)
{
    int r = 0;
    while (v > 1) {
        v >>= 1;
        r++;
    }
    return r;
}

/* 从逻辑层把当前真实棋盘复制到本地数组，AI 只在副本上推演，
 * 绝不影响正在进行的真实对局（也无需操作撤销栈）。
 */
static void snapshot_board(int b[BOARD_SIZE][BOARD_SIZE])
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            b[r][c] = game_get(r, c);
}

/* 本地棋盘的逆序一行 / 转置（与逻辑层同思路，用于复用 slide_left） */
static void local_reverse_row(int row[BOARD_SIZE])
{
    int i;
    for (i = 0; i < BOARD_SIZE / 2; i++) {
        int t = row[i];
        row[i] = row[BOARD_SIZE - 1 - i];
        row[BOARD_SIZE - 1 - i] = t;
    }
}

static void local_transpose(int b[BOARD_SIZE][BOARD_SIZE])
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = r + 1; c < BOARD_SIZE; c++) {
            int t = b[r][c];
            b[r][c] = b[c][r];
            b[c][r] = t;
        }
}

/* 在本地棋盘副本上模拟一个方向的一步移动（不生成随机方块）。
 * 返回合并得分；棋盘无变化（该方向不可走）时返回 -1。
 */
static int local_apply_move(int b[BOARD_SIZE][BOARD_SIZE], int dir)
{
    int before[BOARD_SIZE][BOARD_SIZE];
    int gained = 0;
    int r, c, changed = 0;

    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            before[r][c] = b[r][c];

    switch (dir) {
    case DIR_LEFT:
        for (r = 0; r < BOARD_SIZE; r++)
            slide_left(b[r], &gained);
        break;
    case DIR_RIGHT:
        for (r = 0; r < BOARD_SIZE; r++) {
            local_reverse_row(b[r]);
            slide_left(b[r], &gained);
            local_reverse_row(b[r]);
        }
        break;
    case DIR_UP:
        local_transpose(b);
        for (r = 0; r < BOARD_SIZE; r++)
            slide_left(b[r], &gained);
        local_transpose(b);
        break;
    case DIR_DOWN:
        local_transpose(b);
        for (r = 0; r < BOARD_SIZE; r++) {
            local_reverse_row(b[r]);
            slide_left(b[r], &gained);
            local_reverse_row(b[r]);
        }
        local_transpose(b);
        break;
    default:
        return -1;
    }

    for (r = 0; r < BOARD_SIZE && !changed; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (b[r][c] != before[r][c]) {
                changed = 1;
                break;
            }
    return changed ? gained : -1;
}

/* ===================== 启发式评估函数 ===================== */

/* 对一个棋盘局面打分（越大越好）。四项指标加权：
 * 空格数、平滑度、单调性、最大块是否在角落。全部基于 log2 数值，
 * 使大合并的收益呈线性而非指数，评估更稳定。
 */
static double evaluate(const int b[BOARD_SIZE][BOARD_SIZE])
{
    int empty = 0;
    int max_log = 0, max_r = 0, max_c = 0;
    double smooth_pen = 0.0; /* 相邻 log2 差之和（越小越好） */
    double mono_pen = 0.0;   /* 行/列非单调的代价（越小越好） */
    int r, c, i;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            int v = b[r][c];
            if (v == 0) {
                empty++;
                continue;
            }
            int l = ilog2(v);
            if (l > max_log) {
                max_log = l;
                max_r = r;
                max_c = c;
            }
            /* 平滑度：与右、下邻居的 log2 差 */
            if (c + 1 < BOARD_SIZE && b[r][c + 1] != 0) {
                int d = l - ilog2(b[r][c + 1]);
                smooth_pen += d < 0 ? -d : d;
            }
            if (r + 1 < BOARD_SIZE && b[r + 1][c] != 0) {
                int d = l - ilog2(b[r + 1][c]);
                smooth_pen += d < 0 ? -d : d;
            }
        }
    }

    /* 单调性：对每行（取 log2，空格视为 0）分别计算
     * “递增趋势”与“递减趋势”的违反量，取较小者；
     * 对每列同理。一条完美单调的行/列代价为 0。
     */
    for (r = 0; r < BOARD_SIZE; r++) {
        double inc = 0.0, dec = 0.0;
        int l[BOARD_SIZE];
        for (i = 0; i < BOARD_SIZE; i++)
            l[i] = b[r][i] ? ilog2(b[r][i]) : 0;
        for (i = 0; i + 1 < BOARD_SIZE; i++) {
            if (l[i] > l[i + 1]) dec += l[i] - l[i + 1];
            else                 inc += l[i + 1] - l[i];
        }
        mono_pen += inc < dec ? inc : dec;
    }
    for (c = 0; c < BOARD_SIZE; c++) {
        double inc = 0.0, dec = 0.0;
        int l[BOARD_SIZE];
        for (i = 0; i < BOARD_SIZE; i++)
            l[i] = b[i][c] ? ilog2(b[i][c]) : 0;
        for (i = 0; i + 1 < BOARD_SIZE; i++) {
            if (l[i] > l[i + 1]) dec += l[i] - l[i + 1];
            else                 inc += l[i + 1] - l[i];
        }
        mono_pen += inc < dec ? inc : dec;
    }

    double score = W_EMPTY * empty
                 - W_SMOOTH * smooth_pen
                 - W_MONO * mono_pen;

    /* 最大块在角落时按数值给予额外奖励（经典“压角”策略） */
    if ((max_r == 0 || max_r == BOARD_SIZE - 1) &&
        (max_c == 0 || max_c == BOARD_SIZE - 1))
        score += W_CORNER * max_log;

    return score;
}

/* ===================== Expectimax + Alpha-Beta ===================== */

static double chance_node(int b[BOARD_SIZE][BOARD_SIZE], int depth,
                          double alpha, double beta);

/* Max 层：AI 行动，从四个方向中选期望收益最大者。
 * depth 表示剩余可推演的玩家步数；本层做 Alpha-Beta 剪枝。
 */
static double max_node(int b[BOARD_SIZE][BOARD_SIZE], int depth,
                       double alpha, double beta)
{
    double best = NEG_INF;
    int dir, any = 0;

    if (depth <= 0 || nodes_used >= AI_NODE_BUDGET)
        return evaluate(b);
    nodes_used++;

    for (dir = 0; dir < 4; dir++) {
        int child[BOARD_SIZE][BOARD_SIZE];
        int r, c;
        for (r = 0; r < BOARD_SIZE; r++)
            for (c = 0; c < BOARD_SIZE; c++)
                child[r][c] = b[r][c];

        if (local_apply_move(child, dir) < 0)
            continue; /* 该方向不可走，不是合法着法 */

        any = 1;
        /* 移动后系统随机出块，进入 Chance 层 */
        double val = chance_node(child, depth - 1, alpha, beta);
        if (val > best)
            best = val;
        if (best > alpha)
            alpha = best;
        if (alpha >= beta)
            break; /* Beta 剪枝：对手（此处为上层的概率界）不会再选这条分支 */
    }

    /* 四个方向都不可走 = 死局，给极低分 */
    return any ? best : NEG_INF;
}

/* Chance 层：系统在每个空格随机放 2（概率 0.9）或 4（概率 0.1），
 * 对所有可能局面按概率取期望值。Chance 层不做 Alpha-Beta 剪枝
 * （期望值不是 min/max，剪枝会破坏概率加权的正确性）。
 */
static double chance_node(int b[BOARD_SIZE][BOARD_SIZE], int depth,
                          double alpha, double beta)
{
    int empties[BOARD_SIZE * BOARD_SIZE];
    int n = 0;
    int r, c, i;
    double expected = 0.0;

    if (nodes_used >= AI_NODE_BUDGET)
        return evaluate(b);
    nodes_used++;

    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (b[r][c] == 0)
                empties[n++] = r * BOARD_SIZE + c;

    if (n == 0)
        return max_node(b, depth, alpha, beta); /* 满盘，直接进入下一手 */

    for (i = 0; i < n; i++) {
        int er = empties[i] / BOARD_SIZE;
        int ec = empties[i] % BOARD_SIZE;
        double p_cell = 1.0 / n; /* 方块落在每个空格的概率相同 */

        b[er][ec] = 2;
        expected += p_cell * 0.9 * max_node(b, depth, alpha, beta);
        b[er][ec] = 4;
        expected += p_cell * 0.1 * max_node(b, depth, alpha, beta);
        b[er][ec] = 0; /* 恢复现场，供下一个空格枚举 */

        if (nodes_used >= AI_NODE_BUDGET)
            break;
    }
    return expected;
}

/* ===================== 对外接口 ===================== */

/* 计算当前真实棋盘的最佳移动方向。
 * 采用迭代加深：从深度 1 逐步加深到 AI_MAX_DEPTH，
 * 每加深一层都用更远的搜索结果覆盖之前的答案；
 * 节点预算耗尽时停止加深，保证任何机器上都能瞬间返回。
 */
int ai_best_move(void)
{
    int b[BOARD_SIZE][BOARD_SIZE];
    int best_dir = -1;
    int depth;

    snapshot_board(b);

    for (depth = 1; depth <= AI_MAX_DEPTH; depth++) {
        double best_val = NEG_INF;
        int depth_best = -1;
        int dir;

        nodes_used = 0;
        for (dir = 0; dir < 4; dir++) {
            int child[BOARD_SIZE][BOARD_SIZE];
            int r, c, gained;
            for (r = 0; r < BOARD_SIZE; r++)
                for (c = 0; c < BOARD_SIZE; c++)
                    child[r][c] = b[r][c];

            gained = local_apply_move(child, dir);
            if (gained < 0)
                continue;

            /* 本层评估值 = 立即可得的合并分 + 后续局面的期望值 */
            double val = gained + chance_node(child, depth - 1, NEG_INF, POS_INF);
            if (val > best_val) {
                best_val = val;
                depth_best = dir;
            }
        }

        if (depth_best >= 0)
            best_dir = depth_best; /* 更深的完整搜索覆盖之前的答案 */
        if (nodes_used >= AI_NODE_BUDGET)
            break; /* 预算耗尽，不再加深 */
    }

    return best_dir;
}
