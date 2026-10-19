# ARMOR-HMI host tests: the panel's settings, what it knows of the server, the voice conversation, the board's pins and the screen's words, built with any C++17 compiler.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
cmake_minimum_required(VERSION 3.16)
project(armor_hmi_host_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_executable(test_release test_release.cpp)
add_executable(test_mqtt_tls test_mqtt_tls.cpp)
add_executable(test_server_pin test_server_pin.cpp)
add_executable(test_base test_base.cpp)
add_executable(test_clock test_clock.cpp)
add_executable(test_config test_config.cpp)
add_executable(test_view test_view.cpp)
add_executable(test_voice test_voice.cpp)
add_executable(test_board test_board.cpp)

add_executable(test_logic test_logic.cpp)

# rules shared with the other nodes (ARMOR-COMMON/firmware_base/tests)
add_executable(test_shared test_shared.cpp)

enable_testing()
add_test(NAME release COMMAND test_release)
add_test(NAME mqtt_tls COMMAND test_mqtt_tls)
add_test(NAME server_pin COMMAND test_server_pin)
add_test(NAME base COMMAND test_base)
add_test(NAME clock COMMAND test_clock)
add_test(NAME config COMMAND test_config)
add_test(NAME view COMMAND test_view)
add_test(NAME voice COMMAND test_voice)
add_test(NAME board COMMAND test_board)
add_test(NAME shared COMMAND test_shared)
add_test(NAME logic COMMAND test_logic)

# the panel's texts, and the glyphs of the screen's fonts for every character the screen can draw
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME panel_text COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_panel_text.py)
  # no task keeps a big copy of the settings on its small stack, every task has the stack its job needs, GitHub's long redirects fit the HTTP buffer
  add_test(NAME task_safety COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_task_safety.py)
  # a saved time zone applies at once; the panel can still say that a restart is needed
  add_test(NAME save_paths COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_save_paths.py)
  add_test(NAME font_glyphs COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_font_glyphs.py)
endif()
