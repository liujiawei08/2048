#ifndef UI_H
#define UI_H

/* ========== 渲染 ========== */

/* UI 初始化：Windows 下开启 ANSI 转义序列支持；类 Unix 为空操作。
 * 程序启动时调用一次。
 */
void ui_init(void);

/* 清空屏幕，让每一帧画面固定从顶部开始显示 */
void ui_clear(void);

/* 输出当前棋盘和分数（从 game_logic 模块读取状态，按数值着色） */
void ui_render_board(void);

/* ========== 输入 ========== */

/* 读取一个按键（不回显、不需要回车）。
 * 方向键统一翻译为小写字母 w/a/s/d 返回，其余按键原样返回，
 * 无法识别的功能键返回 0。
 */
int ui_read_key(void);

/* 把按键翻译成方向（game_logic.h 的 DIR_*）；无法识别的按键返回 -1 */
int ui_key_to_dir(int ch);

/* ========== 音效 ========== */

void ui_sound_move(void);     /* 有效移动但未合并：短促低音 */
void ui_sound_merge(void);    /* 发生合并：较高音 */
void ui_sound_undo(void);     /* 撤销成功：下行音 */
void ui_sound_denied(void);   /* 操作被拒绝：低沉警示音 */
void ui_sound_win(void);      /* 首次合成 2048：上行小号角 */
void ui_sound_gameover(void); /* 游戏结束：下行音阶 */

#endif /* UI_H */
