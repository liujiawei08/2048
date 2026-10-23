#ifndef INPUT_H
#define INPUT_H

/* 读取一个按键（不回显、不需要回车）。
 * 方向键统一翻译为小写字母 w/a/s/d 返回，其余按键原样返回，
 * 无法识别的功能键返回 0。
 */
int input_read_key(void);

/* 把按键翻译成方向（game.h 的 DIR_*）；无法识别的按键返回 -1 */
int input_key_to_dir(int ch);

#endif /* INPUT_H */
