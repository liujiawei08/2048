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

    printf("== %d tests, %d passed, %d failed ==\n",
           tests_run, tests_run - tests_failed, tests_failed);
    return tests_failed;
}
