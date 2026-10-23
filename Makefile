# Windows / MinGW: make 生成 game.exe；类 Unix 生成 game
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
# EasyX 只支持 C++，图形版用 g++ 编译
CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2 -Wno-unknown-pragmas

ifeq ($(OS),Windows_NT)
TARGET = game.exe
TEST   = test.exe
else
TARGET = game
TEST   = test
endif

LOGIC = game_logic.c game_logic.h
UI    = ui.c ui.h
AI    = ai.c ai.h

# EasyX 图形版（仅 Windows）：头文件与库放在 third_party/easyx 下
EASYX     ?= third_party/easyx
GUI_TARGET = game_gui.exe

# 游戏本体：主循环 + 逻辑层 + UI 层 + AI 层
$(TARGET): main.c $(LOGIC) $(UI) $(AI)
	$(CC) $(CFLAGS) -o $(TARGET) main.c game_logic.c ui.c ai.c

# 图形版：EasyX 渲染 + 消息循环；-mwindows 去掉多余的控制台窗口
# EasyX 只支持 C++：GUI 两个文件用 g++ 编译，逻辑层仍用 gcc 编成 C 目标文件，
# 头文件里的 extern "C" 保证链接一致。
# 需要先按 README 把 EasyX 的头文件与 libeasyx.a 放到 $(EASYX) 目录
gui: $(GUI_TARGET)

$(GUI_TARGET): main_gui.cpp ui_gui.cpp ui_gui.h $(LOGIC) $(AI)
	$(CC) $(CFLAGS) -c game_logic.c -o game_logic_gui.o
	$(CC) $(CFLAGS) -c ai.c -o ai_gui.o
	$(CXX) $(CXXFLAGS) -I$(EASYX)/include -c main_gui.cpp -o main_gui.o
	$(CXX) $(CXXFLAGS) -I$(EASYX)/include -c ui_gui.cpp -o ui_gui.o
	$(CXX) -o $(GUI_TARGET) main_gui.o ui_gui.o game_logic_gui.o ai_gui.o \
		$(EASYX)/lib/libeasyx.a \
		-lgdi32 -lole32 -lwinmm -luser32 -lmsimg32 -mwindows

# 单元测试只链接逻辑层，不依赖 UI
$(TEST): test.c $(LOGIC)
	$(CC) $(CFLAGS) -o $(TEST) test.c game_logic.c

# 编译并运行单元测试
check: $(TEST)
	./$(TEST)

clean:
	rm -f $(TARGET) $(TEST) $(GUI_TARGET) *.o

.PHONY: check clean gui
