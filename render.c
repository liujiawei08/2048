#include <stdio.h>

#include "render.h"
#include "game.h"

#ifdef _WIN32
#include <stdlib.h>  /* system("cls") */
#include <windows.h> /* 启用 ANSI 转义序列支持 */
#endif

/* ANSI 复位序列：恢复默认前景/背景、关闭加粗 */
#define COLOR_RESET "\033[0m"

/* 渲染初始化：Windows 控制台默认不解释 ANSI 转义序列，需要打开
 * ENABLE_VIRTUAL_TERMINAL_PROCESSING；类 Unix 终端原生支持，无需处理。
 */
void render_init(void)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/* 清空屏幕，让每一帧画面固定从顶部开始显示 */
void render_clear(void)
{
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

/* ANSI 颜色映射表：按方块数值返回“背景色 + 前景色”转义序列。
 * 使用 256 色模式（\033[48;5;Nm 背景、\033[38;5;Nm 前景）：
 * 2 青、4 绿、8 黄，数值越大颜色越暖/越深，2048 红色加粗高亮。
 */
static const char *tile_color(int v)
{
    switch (v) {
    case 0:    return "\033[48;5;236m\033[38;5;250m";        /* 空格：深灰底 */
    case 2:    return "\033[48;5;51m\033[38;5;235m";         /* 青底深字 */
    case 4:    return "\033[48;5;46m\033[38;5;235m";         /* 绿底深字 */
    case 8:    return "\033[48;5;226m\033[38;5;235m";        /* 黄底深字 */
    case 16:   return "\033[48;5;208m\033[38;5;255m";        /* 橙 */
    case 32:   return "\033[48;5;203m\033[38;5;255m";        /* 橙红 */
    case 64:   return "\033[48;5;198m\033[38;5;255m";        /* 品红 */
    case 128:  return "\033[48;5;39m\033[38;5;255m";         /* 蓝 */
    case 256:  return "\033[48;5;27m\033[38;5;255m";         /* 深蓝 */
    case 512:  return "\033[48;5;93m\033[38;5;255m";         /* 紫 */
    case 1024: return "\033[48;5;166m\033[38;5;255m";        /* 铜 */
    case 2048: return "\033[48;5;196m\033[38;5;231m\033[1m"; /* 红底白字加粗：通关高亮 */
    default:   return "\033[48;5;90m\033[38;5;231m\033[1m";  /* >2048：暗紫加粗 */
    }
}

/* 用 printf 格式化输出棋盘：固定宽度保证列对齐，按数值着色。
 * 只通过 game.h 的只读接口取数据，不触碰逻辑层内部状态。
 */
void render_board(void)
{
    int r, c;

    /* 顶栏：当前分数 + 历史最高分（最高分为黄色） */
    printf("Score: %ld    Best: \033[38;5;226m%ld\033[0m\n",
           game_score(), game_high_score());
    printf("+------+------+------+------+\n");
    for (r = 0; r < BOARD_SIZE; r++) {
        printf("|");
        for (c = 0; c < BOARD_SIZE; c++) {
            int v = game_get(r, c);
            printf("%s", tile_color(v));
            if (v == 0)
                printf("      ");
            else
                printf("%6d", v);
            printf(COLOR_RESET "|");
        }
        printf("\n+------+------+------+------+\n");
    }
    printf("Move: w/a/s/d or arrow keys, u = undo, r = restart, q = quit\n");
}
