/* ui_gui.cpp — EasyX 图形界面层实现（EasyX 只支持 C++，故本文件为 .cpp）。
 *
 * 绘制：4x4 棋盘、彩色方块、数字、分数栏、功能按钮、结束遮罩。
 * 输入：EasyX 消息机制（peekmessage 轮询 EX_KEY / EX_MOUSE），
 *       不再使用 getch，也不依赖控制台。
 * 逻辑：只调用 game_logic.h 的公开接口，逻辑层零改动。
 */
#include <stdbool.h> /* 布尔类型声明（C++ 下同样可用） */
#include <graphics.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "ui_gui.h"
#include "game_logic.h"

/* ==================== 界面布局常量 ==================== */

#define WINDOW_W 480
#define WINDOW_H 660

#define MARGIN   20
#define GAP      12
#define CELL     101                        /* 格子边长 */
#define BOARD_X  MARGIN
#define BOARD_Y  96
#define BOARD_PX (BOARD_SIZE * CELL + (BOARD_SIZE - 1) * GAP) /* 440 */

#define BTN_Y   550
#define BTN_H   42
#define BTN_W   101
#define BTN_GAP 12

#define MSG_Y   604

/* ==================== 配色 ==================== */

#define COLOR_BG      RGB(250, 248, 242) /* 窗口底色（避开 windows.h 的 COLOR_WINDOW 宏） */
#define COLOR_BOARD   RGB(187, 173, 160) /* 棋盘底板 */
#define COLOR_EMPTY   RGB(205, 193, 180) /* 空格子 */
#define COLOR_LABEL   RGB(238, 228, 218) /* 分数栏文字 */
#define COLOR_HINT    RGB(140, 130, 120)
#define COLOR_BOX     RGB(160, 145, 130) /* 分数栏/按钮底色 */

/* 按数值返回方块背景色：2 青、4 绿、8 黄，数值越大越暖/越深，
 * 2048 红色高亮，超过 2048 暗紫。与终端版配色语义一致。
 */
static COLORREF tile_bg(int v)
{
    switch (v) {
    case 0:    return COLOR_EMPTY;
    case 2:    return RGB(138, 205, 219);  /* 青 */
    case 4:    return RGB(130, 200, 140);  /* 绿 */
    case 8:    return RGB(240, 215, 90);   /* 黄 */
    case 16:   return RGB(245, 175, 95);   /* 橙 */
    case 32:   return RGB(245, 140, 85);
    case 64:   return RGB(240, 100, 75);
    case 128:  return RGB(135, 180, 235);  /* 蓝 */
    case 256:  return RGB(95, 145, 225);
    case 512:  return RGB(160, 120, 215);  /* 紫 */
    case 1024: return RGB(220, 165, 75);   /* 铜 */
    case 2048: return RGB(230, 65, 50);    /* 红：通关高亮 */
    default:   return RGB(110, 70, 130);   /* >2048：暗紫 */
    }
}

/* 方块数字颜色：浅色块用深字，深色块用白字 */
static COLORREF tile_fg(int v)
{
    if (v <= 8)
        return RGB(70, 60, 55);
    return RGB(255, 255, 255);
}

/* ==================== 绘图小工具 ==================== */

/* 以 (cx, cy) 为中心输出文本（当前字体设置下测量宽高） */
static void centered_text(int cx, int cy, const char *s)
{
    outtextxy(cx - textwidth(s) / 2, cy - textheight(s) / 2, s);
}

/* 画一个圆角矩形（填充色与边框同色） */
static void fill_round_rect(int x, int y, int w, int h, COLORREF color, int radius)
{
    setfillcolor(color);
    setlinecolor(color);
    solidroundrect(x, y, x + w, y + h, radius, radius);
}

/* 第 i 个功能按钮的左上角 x 坐标 */
static int button_x(int i)
{
    return MARGIN + i * (BTN_W + BTN_GAP);
}

/* 鼠标坐标命中的按钮；没命中返回 GUI_BTN_NONE */
static GuiButton hit_button(int x, int y)
{
    int i;
    if (y < BTN_Y || y > BTN_Y + BTN_H)
        return GUI_BTN_NONE;
    for (i = 0; i < 4; i++) {
        int bx = button_x(i);
        if (x >= bx && x <= bx + BTN_W)
            return (GuiButton)i;
    }
    return GUI_BTN_NONE;
}

/* ==================== 各区块绘制 ==================== */

/* 顶部分数栏：标签 + 数值 */
static void draw_score_box(int x, int y, int w, int h, const char *label, long value)
{
    char buf[32];
    fill_round_rect(x, y, w, h, COLOR_BOX, 8);

    setbkmode(TRANSPARENT);
    settextcolor(COLOR_LABEL);
    settextstyle(14, 0, "Segoe UI");
    centered_text(x + w / 2, y + 16, label);

    snprintf(buf, sizeof buf, "%ld", value);
    settextstyle(24, 0, "Consolas");
    centered_text(x + w / 2, y + 38, buf);
}

/* 单个格子（含数字） */
static void draw_tile(int r, int c)
{
    int v = game_get(r, c);
    int x = BOARD_X + c * (CELL + GAP);
    int y = BOARD_Y + r * (CELL + GAP);
    int h;

    fill_round_rect(x, y, CELL, CELL, tile_bg(v), 12);
    if (v == 0)
        return;

    /* 数字越多位，字号越小，保证不溢出格子 */
    if (v < 100)       h = 40;
    else if (v < 1000) h = 32;
    else if (v < 10000)h = 26;
    else               h = 21;

    char buf[16];
    snprintf(buf, sizeof buf, "%d", v);
    setbkmode(TRANSPARENT);
    settextcolor(tile_fg(v));
    settextstyle(h, 0, "Consolas");
    centered_text(x + CELL / 2, y + CELL / 2, buf);
}

/* 整个棋盘 */
static void draw_board(void)
{
    int r, c;

    fill_round_rect(BOARD_X - 8, BOARD_Y - 8, BOARD_PX + 16, BOARD_PX + 16,
                    COLOR_BOARD, 16);
    for (r = 0; r < BOARD_SIZE; r++)
        for (c = 0; c < BOARD_SIZE; c++)
            draw_tile(r, c);
}

/* 功能按钮栏 */
static void draw_buttons(void)
{
    static const char *labels[4] = { "Undo (U)", "Restart (R)", "AI (A)", "Quit (Q)" };
    int i;

    setbkmode(TRANSPARENT);
    for (i = 0; i < 4; i++) {
        fill_round_rect(button_x(i), BTN_Y, BTN_W, BTN_H, COLOR_BOX, 8);
        settextcolor(COLOR_LABEL);
        settextstyle(15, 0, "Segoe UI");
        centered_text(button_x(i) + BTN_W / 2, BTN_Y + BTN_H / 2, labels[i]);
    }
}

/* 游戏结束遮罩：盖在棋盘中央 */
static void draw_gameover_overlay(void)
{
    char buf[48];
    int x = BOARD_X + 24;
    int y = BOARD_Y + 120;
    int w = BOARD_PX - 48;
    int h = 150;

    fill_round_rect(x, y, w, h, RGB(250, 248, 242), 14);
    setbkmode(TRANSPARENT);
    settextcolor(RGB(119, 110, 101));
    settextstyle(34, 0, "Segoe UI");
    centered_text(x + w / 2, y + 44, "Game Over");

    snprintf(buf, sizeof buf, "Final score: %ld", game_score());
    settextstyle(18, 0, "Consolas");
    centered_text(x + w / 2, y + 88, buf);

    settextstyle(15, 0, "Segoe UI");
    settextcolor(RGB(140, 130, 120));
    centered_text(x + w / 2, y + 122, "Press R to restart, or click Restart");
}

/* ==================== 对外接口 ==================== */

void gui_init(void)
{
    initgraph(WINDOW_W, WINDOW_H, 0);
    SetWindowText(GetHWnd(), "2048 - EasyX");
    setbkcolor(COLOR_BG);
    cleardevice();
    BeginBatchDraw();
}

void gui_shutdown(void)
{
    EndBatchDraw();
    closegraph();
}

void gui_draw(const char *msg)
{
    setbkcolor(COLOR_BG);
    cleardevice();

    /* 标题 */
    setbkmode(TRANSPARENT);
    settextcolor(RGB(119, 110, 101));
    settextstyle(44, 0, "Segoe UI");
    outtextxy(MARGIN, 24, "2048");

    /* 分数栏 */
    draw_score_box(236, 24, 104, 56, "SCORE", game_score());
    draw_score_box(348, 24, 112, 56, "BEST", game_high_score());

    /* 棋盘与按钮 */
    draw_board();
    draw_buttons();

    /* 底部提示：有消息显示消息，否则显示按键说明 */
    setbkmode(TRANSPARENT);
    if (msg != NULL && msg[0] != '\0') {
        settextcolor(RGB(90, 80, 72));
        settextstyle(16, 0, "Segoe UI");
        centered_text(WINDOW_W / 2, MSG_Y + 10, msg);
    } else {
        settextcolor(COLOR_HINT);
        settextstyle(14, 0, "Segoe UI");
        centered_text(WINDOW_W / 2, MSG_Y + 10,
                      "Arrows/WSD move  ·  A = AI step  ·  U undo  ·  R restart  ·  Q quit");
    }

    if (game_over())
        draw_gameover_overlay();

    FlushBatchDraw();
}

GuiEvent gui_next_event(void)
{
    GuiEvent ev;
    ExMessage msg;

    ev.kind = GUI_EVENT_NONE;
    ev.key = 0;
    ev.button = GUI_BTN_NONE;

    if (peekmessage(&msg, EX_KEY | EX_MOUSE, true)) {
        if (msg.message == WM_KEYDOWN) {
            ev.kind = GUI_EVENT_KEY;
            ev.key = msg.vkcode; /* 字母键为 'A'~'Z'，方向键为 VK_* */
            return ev;
        }
        if (msg.message == WM_LBUTTONDOWN) {
            ev.kind = GUI_EVENT_BUTTON;
            ev.button = hit_button(msg.x, msg.y);
            return ev;
        }
    }

    /* 用户点了窗口右上角的 X：EasyX 不会自动退出程序，需要自己检测 */
    if (!IsWindow(GetHWnd())) {
        ev.kind = GUI_EVENT_CLOSE;
        return ev;
    }

    Sleep(10); /* 没有事件时让出 CPU，避免空转 */
    return ev;
}

void gui_sound(int kind)
{
    switch (kind) {
    case GUI_SND_MOVE:     Beep(440, 40); break;
    case GUI_SND_MERGE:    Beep(660, 60); break;
    case GUI_SND_UNDO:     Beep(330, 50); break;
    case GUI_SND_DENIED:   Beep(150, 100); break;
    case GUI_SND_WIN:
        Beep(523, 80); Beep(659, 80); Beep(784, 80); Beep(1047, 160);
        break;
    case GUI_SND_GAMEOVER:
        Beep(400, 120); Beep(300, 120); Beep(200, 200);
        break;
    default: break;
    }
}
