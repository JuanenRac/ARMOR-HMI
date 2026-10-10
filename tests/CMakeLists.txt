# ARMOR-HMI host tests: the panel's settings, what it knows of the server, the voice conversation, the board's pins and the screen's words, built with any C++17 compiler.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
cmake_minimum_required(VERSION 3.16)
project(armor_hmi_host_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_executable(test_release test_release.cpp)
add_executable(test_clock test_clock.cpp)
add_executable(test_config test_config.cpp)
add_executable(test_view test_view.cpp)
add_executable(test_voice test_voice.cpp)
add_executable(test_board test_board.cpp)

enable_testing()
add_test(NAME release COMMAND test_release)
add_test(NAME clock COMMAND test_clock)
add_test(NAME config COMMAND test_config)
add_test(NAME view COMMAND test_view)
add_test(NAME voice COMMAND test_voice)
add_test(NAME board COMMAND test_board)
