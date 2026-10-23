#ifndef UI_GUI_H
#define UI_GUI_H

/* ============================================================
 * ui_gui.h — EasyX 图形界面层（Windows）
 *
 * 与终端版 ui.c 完全独立：不使用 printf、不使用 ANSI 转义序列，
 * 所有画面由 EasyX 绘制；输入来自图形窗口的键盘/鼠标消息。
 * 逻辑层 game_logic 保持原样，通过公开接口读写棋盘。
 * ============================================================ */

/* 事件类型 */
typedef enum {
    GUI_EVENT_NONE = 0, /* 这一轮没有新事件（轮询超时） */
    GUI_EVENT_KEY,      /* 键盘按下：key 为虚拟键码（VK_*） */
    GUI_EVENT_BUTTON,   /* 鼠标点击按钮：button 为按钮编号 */
    GUI_EVENT_CLOSE     /* 窗口被关闭 */
} GuiEventKind;

/* 界面按钮编号 */
typedef enum {
    GUI_BTN_NONE = -1,
    GUI_BTN_UNDO = 0,
    GUI_BTN_RESTART,
    GUI_BTN_AI,
    GUI_BTN_QUIT
} GuiButton;

typedef struct {
    GuiEventKind kind;
    int key;          /* 仅 GUI_EVENT_KEY 有效 */
    GuiButton button; /* 仅 GUI_EVENT_BUTTON 有效 */
} GuiEvent;

/* 音效种类 */
enum {
    GUI_SND_MOVE = 0,
    GUI_SND_MERGE,
    GUI_SND_UNDO,
    GUI_SND_DENIED,
    GUI_SND_WIN,
    GUI_SND_GAMEOVER
};

/* 创建窗口并初始化绘图环境；程序启动时调用一次 */
void gui_init(void);

/* 关闭窗口、释放绘图环境 */
void gui_shutdown(void);

/* 重绘整个界面（棋盘、分数、按钮、状态提示）。
 * msg 为底部提示文字，可为空字符串；界面状态全部从 game_logic 读取。
 */
void gui_draw(const char *msg);

/* 非阻塞取一个事件；没有事件时短暂休眠后返回 GUI_EVENT_NONE，
 * 同时检测窗口是否已被用户关闭。
 */
GuiEvent gui_next_event(void);

/* 播放音效（Windows Beep，不依赖终端） */
void gui_sound(int kind);

#endif /* UI_GUI_H */
