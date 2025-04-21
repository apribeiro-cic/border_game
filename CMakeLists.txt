
cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)

project(border_game C CXX ASM)
pico_sdk_init()

add_executable(${PROJECT_NAME} border_game.c lib/ssd1306.c )

pico_enable_stdio_uart(border_game 1)
pico_enable_stdio_usb(border_game 1)

pico_generate_pio_header(border_game ${CMAKE_CURRENT_LIST_DIR}/border_game.pio)
target_sources(border_game PRIVATE border_game.c)

target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_pwm
        hardware_pio)

pico_add_extra_outputs(border_game)
