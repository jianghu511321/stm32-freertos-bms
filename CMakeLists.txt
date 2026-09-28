cmake_minimum_required(VERSION 3.16)
project(stm32_freertos_bms_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(bms_core
    firmware/App/Src/bms_can_protocol.c
    firmware/App/Src/bms_filter.c
    firmware/App/Src/bms_protection.c
    firmware/App/Src/bms_sensor_sim.c
)
target_include_directories(bms_core PUBLIC firmware/App/Inc)
target_compile_options(bms_core PRIVATE -Wall -Wextra -Werror -Wpedantic)

add_executable(bms_tests tests/test_bms.c)
target_link_libraries(bms_tests PRIVATE bms_core)
target_compile_options(bms_tests PRIVATE -Wall -Wextra -Werror -Wpedantic)

enable_testing()
add_test(NAME bms_tests COMMAND bms_tests)

