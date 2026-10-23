#include <stdio.h>

#include "input.h"
#include "game.h"

#ifdef _WIN32
#include <conio.h> /* _getch */
#else
#include <termios.h>
#include <unistd.h>
#endif

/* 读取一个按键（不回显、不需要回车）。
 * Windows 用 conio.h 的 _getch；类 Unix 用 termios 关闭规范模式实现。
 * 方向键统一翻译为小写字母 w/a/s/d 返回，主循环只需处理一套按键。
 */
int input_read_key(void)
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
int input_key_to_dir(int ch)
{
    switch (ch) {
    case 'a': case 'A': return DIR_LEFT;
    case 'd': case 'D': return DIR_RIGHT;
    case 'w': case 'W': return DIR_UP;
    case 's': case 'S': return DIR_DOWN;
    default:            return -1;
    }
}
