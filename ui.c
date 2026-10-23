/* ui.c — UI 层：终端渲染、键盘输入、音效反馈。
 * 只通过 game_logic.h 的公开接口读写游戏状态，不接触逻辑层内部。
 */
#include <stdio.h>

#include "ui.h"
#include "game_logic.h"

#ifdef _WIN32
#include <conio.h>   /* _getch */
#include <stdlib.h>  /* system("cls") */
#include <windows.h> /* ANSI 支持、Beep */
#else
#include <termios.h>
#include <unistd.h>
#endif

/* ANSI 复位序列：恢复默认前景/背景、关闭加粗 */
#define COLOR_RESET "\033[0m"

/* ==================== 渲染 ==================== */

/* UI 初始化：Windows 控制台默认不解释 ANSI 转义序列，需要打开
 * ENABLE_VIRTUAL_TERMINAL_PROCESSING；类 Unix 终端原生支持，无需处理。
 */
void ui_init(void)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/* 清空屏幕，让每一帧画面固定从顶部开始显示 */
void ui_clear(void)
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
 * 转义序列不占显示宽度，每格仍是 %6d 的 6 字符可见宽度。
 */
void ui_render_board(void)
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

/* ==================== 输入 ==================== */

/* 读取一个按键（不回显、不需要回车）。
 * Windows 用 conio.h 的 _getch；类 Unix 用 termios 关闭规范模式实现。
 * 方向键统一翻译为小写字母 w/a/s/d 返回，主循环只需处理一套按键。
 */
int ui_read_key(void)
{
#ifdef _WIN32
    int ch = _getch();
    if (ch == 0 || ch == 224) { /* 方向键等功能键的前缀 */
        ch = _getch();
        switch (ch) {
        case 72: return 'w'; /* 上 */
        case 75: return 'a'; /* 左 */
        case 77: return 'd'; /* 右 */
        case 80: return 's'; /* 下 */
        default: return 0;
        }
    }
    return ch;
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    ch = getchar();
    if (ch == 27) { /* ESC [ A/B/C/D 方向键转义序列 */
        if (getchar() == '[') {
            switch (getchar()) {
            case 'A': ch = 'w'; break;
            case 'C': ch = 'd'; break;
            case 'B': ch = 's'; break;
            case 'D': ch = 'a'; break;
            default:  ch = 0;   break;
            }
        } else {
            ch = 0;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}

/* 把按键翻译成方向；无法识别的按键返回 -1 */
int ui_key_to_dir(int ch)
{
    switch (ch) {
    case 'a': case 'A': return DIR_LEFT;
    case 'd': case 'D': return DIR_RIGHT;
    case 'w': case 'W': return DIR_UP;
    case 's': case 'S': return DIR_DOWN;
    default:            return -1;
    }
}

/* ==================== 音效 ====================
 * Windows 用 Beep(频率Hz, 时长ms)，时长都 <=80ms 几乎无感；
 * 类 Unix 退化为终端响铃 '\a'，只对重要事件（合并/通关/结束）响铃。
 */

/* 有效移动但未合并：短促低音 */
void ui_sound_move(void)
{
#ifdef _WIN32
    Beep(440, 40);
#endif
}

/* 发生合并：较高音 */
void ui_sound_merge(void)
{
#ifdef _WIN32
    Beep(660, 60);
#else
    putchar('\a');
#endif
}

/* 撤销成功：下行音 */
void ui_sound_undo(void)
{
#ifdef _WIN32
    Beep(330, 50);
#endif
}

/* 操作被拒绝（无法撤销等）：低沉警示音 */
void ui_sound_denied(void)
{
#ifdef _WIN32
    Beep(150, 100);
#endif
}

/* 首次合成 2048：上行小号角 */
void ui_sound_win(void)
{
#ifdef _WIN32
    Beep(523, 80);  /* C5 */
    Beep(659, 80);  /* E5 */
    Beep(784, 80);  /* G5 */
    Beep(1047, 160);/* C6 */
#else
    putchar('\a');
#endif
}

/* 游戏结束：下行音阶 */
void ui_sound_gameover(void)
{
#ifdef _WIN32
    Beep(400, 120);
    Beep(300, 120);
    Beep(200, 200);
#else
    putchar('\a');
#endif
}
