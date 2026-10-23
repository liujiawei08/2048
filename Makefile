# Windows / MinGW: make 生成 game.exe；类 Unix 生成 game
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2

ifeq ($(OS),Windows_NT)
TARGET = game.exe
TEST   = test.exe
else
TARGET = game
TEST   = test
endif

LOGIC = game_logic.c game_logic.h
UI    = ui.c ui.h

# 游戏本体：主循环 + 逻辑层 + UI 层
$(TARGET): main.c $(LOGIC) $(UI)
	$(CC) $(CFLAGS) -o $(TARGET) main.c game_logic.c ui.c

# 单元测试只链接逻辑层，不依赖 UI
$(TEST): test.c $(LOGIC)
	$(CC) $(CFLAGS) -o $(TEST) test.c game_logic.c

# 编译并运行单元测试
check: $(TEST)
	./$(TEST)

clean:
	rm -f $(TARGET) $(TEST)

.PHONY: check clean
