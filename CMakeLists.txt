# 2048（C 语言终端版）跨平台构建方案
# 用法：
#   mkdir build && cd build
#   cmake ..
#   cmake --build .
#   ctest            # 运行单元测试
cmake_minimum_required(VERSION 3.10)

project(game2048 VERSION 1.0 LANGUAGES C)

# 统一使用 C11，关闭编译器私有扩展，保证跨平台行为一致
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 按编译器启用严格警告（与 Makefile 的 -Wall -Wextra 对齐）
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 游戏本体：主循环 + 逻辑层 + UI 层 + AI 层
add_executable(game
    main.c
    game_logic.c
    ui.c
    ai.c
)

# 单元测试：只链接逻辑层，与 UI/AI 完全解耦
add_executable(unit_tests
    test.c
    game_logic.c
)

# 接入 CTest：ctest 或 cmake --build . --target test 均可运行
enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)

# Windows 下 MinGW 静态链接运行库，生成的 game.exe 可脱离 MinGW 环境运行
if(WIN32 AND MINGW)
    target_link_options(game PRIVATE -static)
    target_link_options(unit_tests PRIVATE -static)
endif()
