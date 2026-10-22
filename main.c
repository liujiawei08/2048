#include <stdio.h>

#include "game.h"

#ifdef _WIN32
#include <conio.h>   /* _getch */
#include <stdlib.h>  /* system("cls") */
#include <windows.h> /* 启用 ANSI 转义序列支持 */
#else
#include <termios.h>
#include <unistd.h>
#endif

/* Windows 控制台默认不解释 ANSI 转义序列，需要打开
 * ENABLE_VIRTUAL_TERMINAL_PROCESSING；类 Unix 终端原生支持，无需处理。
 */
static void enable_ansi_colors(void)
{
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
}

/* 读取一个按键（不回显、不需要回车）。
 * Windows 用 conio.h 的 _getch；类 Unix 用 termios 关闭规范模式实现。
 * 方向键统一翻译为小写字母 w/a/s/d 返回，主循环只需处理一套按键。
 */
static int read_key(void)
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

/* 清空屏幕，让每一帧画面固定从顶部开始显示 */
static void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

/* 把按键翻译成方向；无法识别的按键返回 -1 */
static int key_to_dir(int ch)
{
    switch (ch) {
    case 'a': case 'A': return DIR_LEFT;
    case 'd': case 'D': return DIR_RIGHT;
    case 'w': case 'W': return DIR_UP;
    case 's': case 'S': return DIR_DOWN;
    default:            return -1;
    }
}

/* 程序入口：初始化后进入“显示 -> 读键 -> 移动”的主循环 */
int main(void)
{
    int won_shown = 0; /* 胜利提示只显示一次，之后可继续挑战更高分 */

    enable_ansi_colors();
    game_init();

    for (;;) {
        int ch, dir;

        clear_screen();
        game_print();

        if (game_won() && !won_shown) {
            printf("You made 2048! Keep going or press q to quit.\n");
            won_shown = 1;
        }
        if (game_over()) {
            printf("Game over! Final score: %ld\n", game_score());
            printf("Press r to restart, any other key to quit.\n");
            ch = read_key();
            if (ch == 'r' || ch == 'R') {
                game_init();
                won_shown = 0;
                continue;
            }
            break;
        }

        ch = read_key();
        if (ch == 'q' || ch == 'Q')
            break;
        if (ch == 'r' || ch == 'R') {
            game_init();
            won_shown = 0;
            continue;
        }
        if (ch == 'u' || ch == 'U') {
            /* 撤销一步；若棋盘上不再有 2048，允许下次达成时再次提示 */
            if (game_undo() && !game_won())
                won_shown = 0;
            continue;
        }

        dir = key_to_dir(ch);
        if (dir >= 0)
            game_move(dir); /* 返回 0 表示该方向无效，直接重绘即可 */
    }

    clear_screen();
    printf("Bye! Final score: %ld\n", game_score());
    return 0;
}
