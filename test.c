/* test.c — 2048 合并逻辑单元测试
 *
 * 编译（Windows / MinGW）:
 *   gcc -Wall -Wextra -std=c11 -O2 -o test.exe test.c game.c
 * 编译（Linux / macOS）:
 *   gcc -Wall -Wextra -std=c11 -O2 -o test test.c game.c
 * 运行:
 *   ./test.exe   （或 ./test）
 *
 * 退出码：全部通过返回 0，有用例失败返回失败个数。
 */
#include <stdio.h>

#include "game.h"

static int tests_run = 0;
static int tests_failed = 0;

/* 比较一行实际输出与期望输出，并校验合并得分 */
static void check_row(const char *name,
                      const int input[SIZE],
                      const int expect[SIZE],
                      int expect_gained)
{
    int b[SIZE];
    int gained = 0;
    int i, ok = 1;

    for (i = 0; i < SIZE; i++)
        b[i] = input[i];

    slide_left(b, &gained);

    for (i = 0; i < SIZE; i++)
        if (b[i] != expect[i])
            ok = 0;
    if (gained != expect_gained)
        ok = 0;

    tests_run++;
    if (!ok) {
        tests_failed++;
        printf("FAIL  %-28s got [%d %d %d %d] +%d, expect [%d %d %d %d] +%d\n",
               name,
               b[0], b[1], b[2], b[3], gained,
               expect[0], expect[1], expect[2], expect[3], expect_gained);
    } else {
        printf("PASS  %-28s [%d %d %d %d] -> [%d %d %d %d] +%d\n",
               name,
               input[0], input[1], input[2], input[3],
               b[0], b[1], b[2], b[3], gained);
    }
}

/* 验证 gained 传 NULL 时不崩溃（防御性用例） */
static void test_null_gained(void)
{
    int b[SIZE] = {2, 2, 0, 0};
    tests_run++;
    slide_left(b, NULL); /* 不应段错误 */
    if (b[0] == 4 && b[1] == 0 && b[2] == 0 && b[3] == 0) {
        printf("PASS  %-28s no crash, row correct\n", "null gained pointer");
    } else {
        tests_failed++;
        printf("FAIL  %-28s got [%d %d %d %d]\n",
               "null gained pointer", b[0], b[1], b[2], b[3]);
    }
}

/* 整局冒烟测试：初始化后四个方向各走一步，不应崩溃或出现非法状态 */
static void test_smoke_moves(void)
{
    int r, c, bad = 0;
    tests_run++;

    game_init();
    game_move(DIR_LEFT);
    game_move(DIR_RIGHT);
    game_move(DIR_UP);
    game_move(DIR_DOWN);

    /* 棋盘上的值只能是 0 或 2 的幂 */
    for (r = 0; r < SIZE; r++)
        for (c = 0; c < SIZE; c++) {
            int v = game_get(r, c);
            if (v < 0 || (v != 0 && (v & (v - 1)) != 0))
                bad = 1;
        }

    if (bad || game_over()) {
        tests_failed++;
        printf("FAIL  %-28s bad=%d over=%d\n", "smoke four moves", bad, game_over());
    } else {
        printf("PASS  %-28s board legal, score=%ld\n", "smoke four moves", game_score());
    }
}

/* 把当前棋盘拷贝到 out（通过公开接口 game_get 逐格读取） */
static void snapshot_board(int out[SIZE][SIZE])
{
    int r, c;
    for (r = 0; r < SIZE; r++)
        for (c = 0; c < SIZE; c++)
            out[r][c] = game_get(r, c);
}

/* 比较当前棋盘是否与快照一致 */
static int board_matches(const int snap[SIZE][SIZE])
{
    int r, c;
    for (r = 0; r < SIZE; r++)
        for (c = 0; c < SIZE; c++)
            if (game_get(r, c) != snap[r][c])
                return 0;
    return 1;
}

/* 依次尝试四个方向，走出一步有效移动；全都无效返回 0 */
static int make_one_move(void)
{
    static const int dirs[4] = {DIR_LEFT, DIR_UP, DIR_RIGHT, DIR_DOWN};
    int i;
    for (i = 0; i < 4; i++)
        if (game_move(dirs[i]))
            return 1;
    return 0;
}

/* 撤销测试 1：历史为空时 game_undo 必须返回 0 且不破坏棋盘 */
static void test_undo_empty_history(void)
{
    int snap[SIZE][SIZE];
    tests_run++;

    game_init(); /* 重开会清空历史 */
    snapshot_board(snap);

    if (game_undo() != 0 || !board_matches(snap)) {
        tests_failed++;
        printf("FAIL  %-28s undo on empty history misbehaved\n",
               "undo empty history");
    } else {
        printf("PASS  %-28s returns 0, board untouched\n", "undo empty history");
    }
}

/* 撤销测试 2：走一步有效移动后撤销，棋盘和分数都要回到移动前 */
static void test_undo_restores_state(void)
{
    int snap[SIZE][SIZE];
    tests_run++;

    game_init();
    snapshot_board(snap);

    if (!make_one_move()) {
        tests_failed++;
        printf("FAIL  %-28s cannot make any move after init\n",
               "undo restores state");
        return;
    }

    if (game_undo() != 1 || !board_matches(snap) || game_score() != 0) {
        tests_failed++;
        printf("FAIL  %-28s board/score not restored (score=%ld)\n",
               "undo restores state", game_score());
    } else {
        printf("PASS  %-28s board & score restored\n", "undo restores state");
    }
}

/* 撤销测试 3：连续走两步再连撤两步，应回到最初状态 */
static void test_undo_two_steps(void)
{
    int snap[SIZE][SIZE];
    tests_run++;

    game_init();
    snapshot_board(snap);

    if (!make_one_move() || !make_one_move()) {
        tests_failed++;
        printf("FAIL  %-28s cannot make two moves after init\n",
               "undo two steps");
        return;
    }

    if (game_undo() != 1 || game_undo() != 1 ||
        !board_matches(snap) || game_score() != 0) {
        tests_failed++;
        printf("FAIL  %-28s not back to initial state\n", "undo two steps");
    } else {
        printf("PASS  %-28s back to initial state\n", "undo two steps");
    }
}

/* 测试入口：逐个注册用例并汇总结果 */
int main(void)
{
    printf("== slide_left unit tests ==\n");

    /* 题目要求的四个核心用例 */
    check_row("plain move no merge",  (int[]){2, 0, 4, 2}, (int[]){2, 4, 2, 0}, 0);
    check_row("single merge",         (int[]){2, 2, 4, 0}, (int[]){4, 4, 0, 0}, 4);
    check_row("no chain merge",       (int[]){2, 2, 2, 2}, (int[]){4, 4, 0, 0}, 8);
    check_row("separated equals",     (int[]){2, 4, 2, 0}, (int[]){2, 4, 2, 0}, 0);

    /* 补充边界用例 */
    check_row("empty row",            (int[]){0, 0, 0, 0}, (int[]){0, 0, 0, 0}, 0);
    check_row("all distinct",         (int[]){2, 4, 8, 16}, (int[]){2, 4, 8, 16}, 0);
    check_row("triple keeps third",   (int[]){4, 4, 4, 0}, (int[]){8, 4, 0, 0}, 8);
    check_row("two pairs",            (int[]){2, 2, 4, 4}, (int[]){4, 8, 0, 0}, 12);
    check_row("merge after compress", (int[]){0, 2, 0, 2}, (int[]){4, 0, 0, 0}, 4);
    check_row("no chain after merge", (int[]){4, 4, 8, 0}, (int[]){8, 8, 0, 0}, 8);
    check_row("big values",           (int[]){1024, 1024, 0, 0}, (int[]){2048, 0, 0, 0}, 2048);
    check_row("trailing pair",        (int[]){0, 0, 8, 8}, (int[]){16, 0, 0, 0}, 16);

    test_null_gained();
    test_smoke_moves();

    printf("== undo tests ==\n");
    test_undo_empty_history();
    test_undo_restores_state();
    test_undo_two_steps();

    printf("== %d tests, %d passed, %d failed ==\n",
           tests_run, tests_run - tests_failed, tests_failed);
    return tests_failed;
}
