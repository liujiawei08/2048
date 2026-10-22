# Windows / MinGW: make 后生成 2048.exe；类 Unix 生成 2048
CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2

ifeq ($(OS),Windows_NT)
TARGET = 2048.exe
else
TARGET = 2048
endif

$(TARGET): main.c game.c game.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c game.c

clean:
	rm -f $(TARGET)

.PHONY: clean
