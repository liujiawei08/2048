/* main_gui.c — EasyX 图形版入口与主循环。
 *
 * 与终端版 main.c 的职责完全一致：只做流程编排。
 * 区别在于事件源换成图形窗口的键盘/鼠标消息：
 *   - 方向键 或 W/S/D 移动（A 键用于 AI，左移请用 ←）
 *   - A 键或点击 "AI (A)" 按钮：AI 自动走一步
 *   - U 撤销，R 重开，Q / Esc 退出，也可直接点击对应按钮
 */
#include <stdio.h>
#include <string.h>
#include <windows.h> /* VK_* 虚拟键码 */

#include "game_logic.h"
#include "ui_gui.h"
#include "ai.h"

/* 主循环需要跨事件保留的状态 */
typedef struct {
    int won_shown;    /* 2048 胜利提示是否已显示过 */
    int over_sounded; /* 终局音效是否已播放过，避免每帧重复 */
    char msg[80];     /* 底部提示文字 */
} Ctx;

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

/* 执行一步移动（玩家或 AI 共用），负责音效与提示 */
static void do_move(int dir, Ctx *ctx)
{
    long before;
    int over_now;

    if (game_over()) {
        snprintf(ctx->msg, sizeof ctx->msg,
                 "Game over - press R or click Restart.");
        gui_sound(GUI_SND_DENIED);
        return;
    }

    before = game_score();
    if (!game_move(dir)) {
        snprintf(ctx->msg, sizeof ctx->msg, "That direction does nothing.");
        return;
    }

    /* 按分数是否增长区分“普通移动”和“发生合并”两种音效 */
    if (game_score() > before)
        gui_sound(GUI_SND_MERGE);
    else
        gui_sound(GUI_SND_MOVE);

    ctx->msg[0] = '\0';

    if (game_won() && !ctx->won_shown) {
        snprintf(ctx->msg, sizeof ctx->msg,
                 "You made 2048! Keep going.");
        gui_sound(GUI_SND_WIN);
        ctx->won_shown = 1;
    }

    over_now = game_over();
    if (over_now && !ctx->over_sounded) {
        gui_sound(GUI_SND_GAMEOVER);
        ctx->over_sounded = 1;
    }
}

/* AI 自动走一步 */
static void do_ai(Ctx *ctx)
{
    int dir;
    long before;
    int over_now;

    if (game_over()) {
        snprintf(ctx->msg, sizeof ctx->msg,
                 "Game over - press R or click Restart.");
        gui_sound(GUI_SND_DENIED);
        return;
    }

    dir = ai_best_move();
    if (dir < 0) {
        snprintf(ctx->msg, sizeof ctx->msg, "AI: no available move.");
        gui_sound(GUI_SND_DENIED);
        return;
    }

    before = game_score();
    game_move(dir);
    if (game_score() > before)
        gui_sound(GUI_SND_MERGE);
    else
        gui_sound(GUI_SND_MOVE);

    snprintf(ctx->msg, sizeof ctx->msg, "AI moved: %s", dir_name(dir));

    if (game_won() && !ctx->won_shown) {
        snprintf(ctx->msg, sizeof ctx->msg,
                 "You made 2048! Keep going.");
        gui_sound(GUI_SND_WIN);
        ctx->won_shown = 1;
    }

    over_now = game_over();
    if (over_now && !ctx->over_sounded) {
        gui_sound(GUI_SND_GAMEOVER);
        ctx->over_sounded = 1;
    }
}

/* 撤销一步 */
static void do_undo(Ctx *ctx)
{
    if (game_undo()) {
        if (!game_won())
            ctx->won_shown = 0; /* 盘面回到未通关状态，下次达成可再次提示 */
        ctx->over_sounded = 0;  /* 撤销可能让死局复活，允许再次播放终局音 */
        snprintf(ctx->msg, sizeof ctx->msg, "Undone.");
        gui_sound(GUI_SND_UNDO);
    } else {
        snprintf(ctx->msg, sizeof ctx->msg,
                 "Cannot undo: already at the start.");
        gui_sound(GUI_SND_DENIED);
    }
}

/* 重新开始一局 */
static void do_restart(Ctx *ctx)
{
    game_highscore_save(); /* 破纪录则先写盘 */
    game_init();
    ctx->won_shown = 0;
    ctx->over_sounded = 0;
    snprintf(ctx->msg, sizeof ctx->msg, "New game.");
}

int main(void)
{
    Ctx ctx;

    memset(&ctx, 0, sizeof ctx);

    gui_init();
    game_highscore_load();
    game_init();
    gui_draw(ctx.msg);

    for (;;) {
        GuiEvent ev = gui_next_event();
        int dir = -1;
        int quit = 0;

        if (ev.kind == GUI_EVENT_NONE)
            continue;
        if (ev.kind == GUI_EVENT_CLOSE)
            break;
        if (ev.kind == GUI_EVENT_BUTTON && ev.button == GUI_BTN_NONE)
            continue; /* 点在空白处 */

        if (ev.kind == GUI_EVENT_BUTTON) {
            switch (ev.button) {
            case GUI_BTN_UNDO:    do_undo(&ctx); break;
            case GUI_BTN_RESTART: do_restart(&ctx); break;
            case GUI_BTN_AI:      do_ai(&ctx); break;
            case GUI_BTN_QUIT:    quit = 1; break;
            default: break;
            }
        } else {
            switch (ev.key) {
            /* 方向键（推荐，避免与 A=AI 冲突） */
            case VK_LEFT:  dir = DIR_LEFT; break;
            case VK_RIGHT: dir = DIR_RIGHT; break;
            case VK_UP:    dir = DIR_UP; break;
            case VK_DOWN:  dir = DIR_DOWN; break;
            /* 字母移动键：A 被 AI 占用，左移请用 ← */
            case 'W': case 'w': dir = DIR_UP; break;
            case 'S': case 's': dir = DIR_DOWN; break;
            case 'D': case 'd': dir = DIR_RIGHT; break;

            case 'A': case 'a': do_ai(&ctx); break;
            case 'U': case 'u': do_undo(&ctx); break;
            case 'R': case 'r': do_restart(&ctx); break;
            case 'Q': case 'q': case VK_ESCAPE: quit = 1; break;
            default: break;
            }
        }

        if (quit)
            break;
        if (dir >= 0)
            do_move(dir, &ctx);

        gui_draw(ctx.msg);
    }

    game_highscore_save(); /* 退出前保存可能破掉的纪录 */
    gui_shutdown();
    return 0;
}
