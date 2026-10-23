#include <stdio.h>

#include "game_logic.h" /* 逻辑层：棋盘状态与移动 */
#include "ui.h"         /* UI 层：渲染、输入、音效 */
#include "ai.h"         /* AI 层：Expectimax 自动决策 */

/* 方向枚举值对应的显示名（用于 AI 走子提示） */
static const char *dir_name(int dir)
{
    switch (dir) {
    case DIR_LEFT:  return "LEFT";
    case DIR_RIGHT: return "RIGHT";
    case DIR_UP:    return "UP";
    case DIR_DOWN:  return "DOWN";
    default:        return "?";
    }
}

/* 程序入口：初始化后进入“显示 -> 读键 -> 移动”的主循环。
 * 本文件只负责流程编排，所有具体工作委派给两个模块。
 */
int main(void)
{
    int won_shown = 0;      /* 胜利提示只显示一次，之后可继续挑战更高分 */
    char msg[64] = "";      /* 一次性提示信息（如“无法撤销”），下一帧后清空 */

    ui_init();
    game_highscore_load();  /* 从 highscore.txt 载入历史最高分 */
    game_init();

    for (;;) {
        int ch, dir;

        ui_clear();
        ui_render_board();
        if (msg[0] != '\0') {
            printf("%s\n", msg);
            msg[0] = '\0'; /* 提示只显示一帧 */
        }

        if (game_won() && !won_shown) {
            printf("You made 2048! Keep going or press q to quit.\n");
            ui_sound_win();
            won_shown = 1;
        }
        if (game_over()) {
            if (game_highscore_save())
                printf("New record! Best score: %ld\n", game_high_score());
            printf("Game over! Final score: %ld\n", game_score());
            printf("Press r to restart, any other key to quit.\n");
            ui_sound_gameover();
            ch = ui_read_key();
            if (ch == 'r' || ch == 'R') {
                game_init();
                won_shown = 0;
                continue;
            }
            break;
        }

        ch = ui_read_key();
        if (ch == 'q' || ch == 'Q') {
            game_highscore_save(); /* 中途退出也把破过的纪录写盘 */
            break;
        }
        if (ch == 'r' || ch == 'R') {
            game_highscore_save();
            game_init();
            won_shown = 0;
            continue;
        }
        if (ch == 'u' || ch == 'U') {
            if (game_undo()) {
                /* 若棋盘上不再有 2048，允许下次达成时再次提示 */
                if (!game_won())
                    won_shown = 0;
                ui_sound_undo();
            } else {
                snprintf(msg, sizeof msg, "Cannot undo: already at the start.");
                ui_sound_denied();
            }
            continue;
        }

        if (ch == 'a' || ch == 'A') {
            /* AI 自动游玩：Expectimax 计算最佳方向并执行 */
            int ai_dir = ai_best_move();
            if (ai_dir >= 0) {
                long before_score = game_score();
                if (game_move(ai_dir)) {
                    if (game_score() > before_score)
                        ui_sound_merge();
                    else
                        ui_sound_move();
                    snprintf(msg, sizeof msg, "AI moved: %s", dir_name(ai_dir));
                }
            } else {
                snprintf(msg, sizeof msg, "AI: no available move.");
                ui_sound_denied();
            }
            continue;
        }

        dir = ui_key_to_dir(ch);
        if (dir >= 0) {
            /* 通过移动前后的分数变化判断是否发生了合并，播放不同音效 */
            long before_score = game_score();
            if (game_move(dir)) {
                if (game_score() > before_score)
                    ui_sound_merge();
                else
                    ui_sound_move();
            }
        }
    }

    ui_clear();
    printf("Bye! Final score: %ld (Best: %ld)\n", game_score(), game_high_score());
    return 0;
}
