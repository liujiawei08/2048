#ifndef GAME_H
#define GAME_H

/* 棋盘边长（经典 2048 为 4x4） */
#define BOARD_SIZE 4

/* 方向枚举，供 game_move() 使用 */
enum Direction {
    DIR_LEFT = 0,
    DIR_RIGHT,
    DIR_UP,
    DIR_DOWN
};

/* 初始化棋盘：清零并随机生成两个初始方块 */
void game_init(void);

/* 在随机空格生成一个新方块（90% 为 2，10% 为 4）；无空格时不做任何事 */
void game_spawn(void);

/* 对一行（长度为 4 的数组）执行“向左滑动并合并”，
 * 合并产生的分数累加到 *gained（可为 NULL）。
 * 四个方向的移动都通过反转/转置复用本函数。
 */
void slide_left(int b[BOARD_SIZE], int *gained);

/* 向指定方向移动一步。
 * 若棋盘发生变化，返回 1，先把移动前的状态存入撤销历史，
 * 再计分并在随机空格生成新方块；
 * 若该方向无法移动，返回 0，棋盘与历史均不变。
 */
int game_move(int dir);

/* 撤销上一步有效移动：恢复之前的棋盘与分数。
 * 历史为空（无法撤销）时返回 0，成功撤销返回 1。
 */
int game_undo(void);

/* 判断游戏是否结束：无空格且任意相邻方块都不相等时返回 1 */
int game_over(void);

/* 判断是否已经合成出 2048（用于首次胜利提示） */
int game_won(void);

/* 读取某个格子的值（行、列从 0 开始） */
int game_get(int row, int col);

/* 读取当前总分 */
long game_score(void);

/* 从本地文件 highscore.txt 读取历史最高分；文件不存在或损坏时视为 0。
 * 程序启动时调用一次。
 */
void game_highscore_load(void);

/* 读取历史最高分（本局分数超过后会即时更新内存中的值） */
long game_high_score(void);

/* 游戏结束时调用：若当前分数破了纪录则写回文件并返回 1，否则返回 0 */
int game_highscore_save(void);

#endif /* GAME_H */
