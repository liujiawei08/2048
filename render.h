#ifndef RENDER_H
#define RENDER_H

/* 渲染初始化：Windows 下开启 ANSI 转义序列支持；类 Unix 为空操作。
 * 程序启动时调用一次。
 */
void render_init(void);

/* 清空屏幕，让每一帧画面固定从顶部开始显示 */
void render_clear(void);

/* 输出当前棋盘和分数（从 game 模块读取状态，按数值着色） */
void render_board(void);

#endif /* RENDER_H */
