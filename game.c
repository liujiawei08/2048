#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game.h"

/* 撤销历史的最大步数，超出后丢弃最早的一步 */
#define UNDO_MAX 100

/* 一步快照：整个棋盘 + 当时的分数 */
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    long score;
} Snapshot;

/* 棋盘与分数作为模块内部状态，对外只通过 game.h 的函数访问 */
static int board[BOARD_SIZE][BOARD_SIZE];
static long score;

/* 撤销历史栈：history[0] 是最早的一步，history[history_len-1] 是最近的一步 */
static Snapshot history[UNDO_MAX];
static int history_len = 0;

/* 把一行数组逆序（用于复用 slide_left 实现向右移动） */
static void reverse_row(int b[BOARD_SIZE])
{
    int i;
    for (i = 0; i < BOARD_SIZE / 2; i++) {
        int t = b[i];
        b[i] = b[BOARD_SIZE - 1 - i];
        b[BOARD_SIZE - 1 - i] = t;
    }
}

/* 棋盘就地转置（用于复用 slide_left 实现上下移动） */
static void transpose(void)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = r + 1; c < BOARD_SIZE; c++) {
            int t = board[r][c];
            board[r][c] = board[c][r];
            board[c][r] = t;
        }
    }
}

/* 复制整个棋盘 */
static void copy_board(int dst[BOARD_SIZE][BOARD_SIZE], const int src[BOARD_SIZE][BOARD_SIZE])
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            dst[r][c] = src[r][c];
}

/* 比较两个棋盘是否完全相同 */
static int boards_equal(const int a[BOARD_SIZE][BOARD_SIZE], const int b[BOARD_SIZE][BOARD_SIZE])
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (a[r][c] != b[r][c])
                return 0;
    return 1;
}

/* 把一份“棋盘 + 分数”快照压入撤销历史；栈满时丢弃最早的一步 */
static void history_push(const int b[BOARD_SIZE][BOARD_SIZE], long s)
{
    if (history_len == UNDO_MAX) {
        memmove(&history[0], &history[1], sizeof(Snapshot) * (UNDO_MAX - 1));
        history_len--;
    }
    copy_board(history[history_len].board, b);
    history[history_len].score = s;
    history_len++;
}

/* 弹出最近一步快照；历史为空返回 0，成功弹出返回 1 */
static int history_pop(int b[BOARD_SIZE][BOARD_SIZE], long *s)
{
    if (history_len == 0)
        return 0;
    history_len--;
    copy_board(b, (const int (*)[BOARD_SIZE])history[history_len].board);
    *s = history[history_len].score;
    return 1;
}

/* 初始化随机数种子（只应调用一次） */
static void seed_rand_once(void)
{
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = 1;
    }
}

/* 初始化棋盘：清零、分数归零、清空撤销历史，并生成两个初始方块 */
void game_init(void)
{
    int r, c;
    seed_rand_once();
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            board[r][c] = 0;
    score = 0;
    history_len = 0; /* 重开一局后不允许撤销到上一局 */
    game_spawn();
    game_spawn();
}

/* 在随机空格生成一个新方块（90% 为 2，10% 为 4） */
void game_spawn(void)
{
    int empty[BOARD_SIZE * BOARD_SIZE];
    int n = 0;
    int r, c, pick, value;

    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] == 0)
                empty[n++] = r * BOARD_SIZE + c;

    if (n == 0)
        return;

    pick = empty[rand() % n];
    value = (rand() % 10 == 0) ? 4 : 2;
    board[pick / BOARD_SIZE][pick % BOARD_SIZE] = value;
}

/* 核心算法：把一行向左滑动并合并一次。
 * 分两步：先去掉 0 把所有方块压到左边，再从左到右扫描，
 * 相邻相等就合并并跳过后一个元素 —— 跳过这步保证了
 * 一次移动中新生成的方块不会再次参与合并（避免连续合并 bug）。
 */
void slide_left(int b[BOARD_SIZE], int *gained)
{
    int tmp[BOARD_SIZE] = {0, 0, 0, 0};
    int out[BOARD_SIZE] = {0, 0, 0, 0};
    int n = 0, m = 0, i;

    /* 第一步：压缩，非零元素依次靠左 */
    for (i = 0; i < BOARD_SIZE; i++)
        if (b[i] != 0)
            tmp[n++] = b[i];

    /* 第二步：合并，合并成功后 i 额外加 1，跳过后一个元素 */
    for (i = 0; i < n; i++) {
        if (i + 1 < n && tmp[i] == tmp[i + 1]) {
            out[m] = tmp[i] * 2;
            if (gained != NULL)
                *gained += tmp[i] * 2;
            m++;
            i++; /* 关键：一次移动中每个方块最多合并一次 */
        } else {
            out[m++] = tmp[i];
        }
    }

    for (i = 0; i < BOARD_SIZE; i++)
        b[i] = out[i];
}

/* 对整个棋盘执行“向左移动”，返回合并得分 */
static int move_board_left(void)
{
    int r;
    int gained = 0;
    for (r = 0; r < BOARD_SIZE; r++)
        slide_left(board[r], &gained);
    return gained;
}

/* 向指定方向移动：通过转置/反转把四个方向统一成“向左”，
 * 移动完成后比较前后棋盘，有变化才计分并生成新方块。
 */
int game_move(int dir)
{
    int before[BOARD_SIZE][BOARD_SIZE];
    int gained = 0;
    int r;

    copy_board(before, board);

    switch (dir) {
    case DIR_LEFT:
        gained = move_board_left();
        break;
    case DIR_RIGHT:
        /* 每一行反转 -> 左移 -> 反转回来 */
        for (r = 0; r < BOARD_SIZE; r++) {
            reverse_row(board[r]);
            slide_left(board[r], &gained);
            reverse_row(board[r]);
        }
        break;
    case DIR_UP:
        /* 转置后列变行，左移等价于上移 */
        transpose();
        gained = move_board_left();
        transpose();
        break;
    case DIR_DOWN:
        /* 转置 + 右移等价于下移 */
        transpose();
        for (r = 0; r < BOARD_SIZE; r++) {
            reverse_row(board[r]);
            slide_left(board[r], &gained);
            reverse_row(board[r]);
        }
        transpose();
        break;
    default:
        return 0;
    }

    if (boards_equal(before, board))
        return 0; /* 该方向无法移动，棋盘与历史都不变 */

    /* 有效移动：先把移动前的状态压入撤销历史，再计分、生成新方块 */
    history_push((const int (*)[BOARD_SIZE])before, score);
    score += gained;
    game_spawn();
    return 1;
}

/* 撤销上一步有效移动：恢复棋盘与分数 */
int game_undo(void)
{
    long s;
    if (!history_pop(board, &s))
        return 0;
    score = s;
    return 1;
}

/* 判断游戏是否结束：有空格可继续；有相邻相等方块也可继续 */
int game_over(void)
{
    int r, c;

    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] == 0)
                return 0;

    for (r = 0; r < BOARD_SIZE; r++) {
        for (c = 0; c < BOARD_SIZE; c++) {
            if (c + 1 < BOARD_SIZE && board[r][c] == board[r][c + 1])
                return 0;
            if (r + 1 < BOARD_SIZE && board[r][c] == board[r + 1][c])
                return 0;
        }
    }
    return 1;
}

/* 判断棋盘上是否已出现 2048 */
int game_won(void)
{
    int r, c;
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            if (board[r][c] == 2048)
                return 1;
    return 0;
}

/* 读取某个格子的值 */
int game_get(int row, int col)
{
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE)
        return -1;
    return board[row][col];
}

/* 读取当前总分 */
long game_score(void)
{
    return score;
}

/* ANSI 颜色映射表：按方块数值返回“背景色 + 前景色”转义序列。
 * 使用 256 色模式（\033[48;5;Nm 背景、\033[38;5;Nm 前景），
 * 配色参考官方 2048：小号数字浅底深字，大号数字深底白字，
 * 2048 及以上加粗显示。
 */
static const char *tile_color(int v)
{
    switch (v) {
    case 0:    return "\033[48;5;236m\033[38;5;250m";        /* 空格：深灰底 */
    case 2:    return "\033[48;5;255m\033[38;5;235m";        /* 米白底深灰字 */
    case 4:    return "\033[48;5;230m\033[38;5;235m";        /* 浅黄底深灰字 */
    case 8:    return "\033[48;5;214m\033[38;5;255m";        /* 橙底白字 */
    case 16:   return "\033[48;5;208m\033[38;5;255m";        /* 深橙 */
    case 32:   return "\033[48;5;203m\033[38;5;255m";        /* 橙红 */
    case 64:   return "\033[48;5;196m\033[38;5;255m";        /* 红 */
    case 128:  return "\033[48;5;227m\033[38;5;235m";        /* 浅金 */
    case 256:  return "\033[48;5;220m\033[38;5;235m";        /* 金 */
    case 512:  return "\033[48;5;178m\033[38;5;255m";        /* 深金 */
    case 1024: return "\033[48;5;166m\033[38;5;255m";        /* 铜 */
    case 2048: return "\033[48;5;40m\033[38;5;231m\033[1m";  /* 绿底加粗：通关色 */
    default:   return "\033[48;5;90m\033[38;5;231m\033[1m";  /* >2048：紫底加粗 */
    }
}

/* ANSI 复位序列：恢复默认前景/背景、关闭加粗 */
#define COLOR_RESET "\033[0m"

/* 用 printf 格式化输出棋盘：固定宽度保证列对齐，按数值着色 */
void game_print(void)
{
    int r, c;

    printf("Score: %ld\n", game_score());
    printf("+------+------+------+------+\n");
    for (r = 0; r < BOARD_SIZE; r++) {
        printf("|");
        for (c = 0; c < BOARD_SIZE; c++) {
            printf("%s", tile_color(board[r][c]));
            if (board[r][c] == 0)
                printf("      ");
            else
                printf("%6d", board[r][c]);
            printf(COLOR_RESET "|");
        }
        printf("\n+------+------+------+------+\n");
    }
    printf("Move: w/a/s/d or arrow keys, u = undo, r = restart, q = quit\n");
}
