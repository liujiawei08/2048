#ifndef AI_H
#define AI_H

/* AI 自动游玩模块。
 *
 * 算法：Expectimax（Minimax 在随机博弈下的推广）+ Max 层 Alpha-Beta 剪枝。
 *   - Max 层：AI 在四个方向中选择期望收益最大的一步；
 *   - Chance 层：模拟系统随机生成方块（每个空格 90% 出 2、10% 出 4），
 *     对所有可能结果按概率取期望值。
 *
 * 用法：int dir = ai_best_move();
 *   返回 game_logic.h 中 Direction 枚举值（0~3），
 *   当前棋盘已无任何可移动方向时返回 -1。
 *   之后照常调用 game_move(dir) 即可让 AI 的决策生效。
 */
int ai_best_move(void);

#endif /* AI_H */
