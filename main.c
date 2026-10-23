#include <stdio.h>

#include "game.h"    /* 逻辑层：棋盘状态与移动 */
#include "render.h"  /* 渲染层：清屏与棋盘输出 */
#include "input.h"   /* 输入层：按键读取与翻译 */

/* 程序入口：初始化后进入“显示 -> 读键 -> 移动”的主循环。
 * 本文件只负责流程编排，所有具体工作委派给三个模块。
 */
int main(void)
{
    int won_shown = 0; /* 胜利提示只显示一次，之后可继续挑战更高分 */

    render_init();
    game_init();

    for (;;) {
        int ch, dir;

        render_clear();
        render_board();

        if (game_won() && !won_shown) {
            printf("You made 2048! Keep going or press q to quit.\n");
            won_shown = 1;
        }
        if (game_over()) {
            printf("Game over! Final score: %ld\n", game_score());
            printf("Press r to restart, any other key to quit.\n");
            ch = input_read_key();
            if (ch == 'r' || ch == 'R') {
                game_init();
                won_shown = 0;
                continue;
            }
            break;
        }

        ch = input_read_key();
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

        dir = input_key_to_dir(ch);
        if (dir >= 0)
            game_move(dir); /* 返回 0 表示该方向无效，直接重绘即可 */
    }

    render_clear();
    printf("Bye! Final score: %ld\n", game_score());
    return 0;
}
