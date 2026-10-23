# Windows / MinGW: make 后生成 2048.exe；类 Unix 生成 2048
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2

ifeq ($(OS),Windows_NT)
TARGET = 2048.exe
TEST   = test.exe
else
TARGET = 2048
TEST   = test
endif

# 游戏本体：主循环 + 逻辑层 + 渲染层 + 输入层
$(TARGET): main.c game.c render.c input.c game.h render.h input.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c game.c render.c input.c

# 单元测试只链接逻辑层，不依赖渲染和输入
$(TEST): test.c game.c game.h
	$(CC) $(CFLAGS) -o $(TEST) test.c game.c

# 编译并运行单元测试
check: $(TEST)
	./$(TEST)

clean:
	rm -f $(TARGET) $(TEST)

.PHONY: check clean
