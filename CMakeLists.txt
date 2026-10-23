# 2048（C 语言终端版）跨平台构建方案
# 用法：
#   mkdir build && cd build
#   cmake ..
#   cmake --build .
#   ctest            # 运行单元测试
cmake_minimum_required(VERSION 3.10)

project(game2048 VERSION 1.0 LANGUAGES C CXX)

# 统一使用 C11 / C++17，关闭编译器私有扩展，保证跨平台行为一致
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# ------------------------------------------------------------
# EasyX 图形版（仅 Windows，可选目标）
# 需要把 EasyX 的 graphics.h 与 MinGW 版 libeasyx.a 放到：
#   third_party/easyx/include/graphics.h
#   third_party/easyx/lib/libeasyx.a
# 找不到时只跳过图形版，终端版与单元测试不受影响。
# 也可以用 -DEASYX_ROOT=你的路径 指定别处。
# ------------------------------------------------------------
set(EASYX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/easyx"
    CACHE PATH "EasyX 根目录（包含 include/ 与 lib/libeasyx.a）")

if(WIN32 AND EXISTS "${EASYX_ROOT}/include/graphics.h"
        AND EXISTS "${EASYX_ROOT}/lib/libeasyx.a")
    # WIN32 关键字让 MinGW 使用 -mwindows：运行时不再弹出控制台
    add_executable(game_gui WIN32
        main_gui.cpp
        ui_gui.cpp
        game_logic.c
        ai.c
    )
    # EasyX 头文件里的 #pragma comment(lib, ...) 对 GCC 无意义，屏蔽该警告
    if(NOT MSVC)
        target_compile_options(game_gui PRIVATE -Wno-unknown-pragmas)
    endif()
    target_include_directories(game_gui PRIVATE "${EASYX_ROOT}/include")
    target_link_libraries(game_gui PRIVATE
        "${EASYX_ROOT}/lib/libeasyx.a"
        gdi32 ole32 winmm user32 msimg32
    )
    if(MINGW)
        target_link_options(game_gui PRIVATE -static)
    endif()
    message(STATUS "EasyX 已找到：生成图形版目标 game_gui")
else()
    message(STATUS "EasyX 未找到：跳过 game_gui 目标（终端版 game 正常构建）")
endif()

# Windows 下 MinGW 静态链接运行库，生成的 game.exe 可脱离 MinGW 环境运行
if(WIN32 AND MINGW)
    target_link_options(game PRIVATE -static)
    target_link_options(unit_tests PRIVATE -static)
endif()
