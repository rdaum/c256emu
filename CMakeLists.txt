cmake_minimum_required(VERSION 3.10)

# Enable Hunter package management.
include("cmake/HunterGate.cmake")
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.188.tar.gz"
        SHA1 "3bcf0667e771b05946d6a70b88a3e3a568b15452"
)

project(c256emu)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# External dependencies
find_package(PkgConfig)

pkg_check_modules(ADPLUG REQUIRED adplug)
pkg_check_modules(SREC REQUIRED srecord)

hunter_add_package(SDL2)
find_package(SDL2 CONFIG REQUIRED)

hunter_add_package(gflags)
find_package(gflags CONFIG REQUIRED)

hunter_add_package(glog)
find_package(glog CONFIG REQUIRED)

hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)

hunter_add_package(Lua)
find_package(Lua CONFIG REQUIRED)

# Configure liblinenoise-ng as a downloadable library.
include(cmake/External_LinenoiseNG.cmake)

# Configure circular_buffer a downloadable library.
include(cmake/External_circular_buffer.cmake)

add_subdirectory(retro_cpu)

message(PUBLIC " ${gflags_INCLUDE_DIRS} -- ${GLOG_INCLUDE_DIR}")
# The c256 bus devices
set(BUS_SOURCES
        src/bus/automation.cc
        src/bus/ch376_sd.cc
        src/bus/int_controller.cc
        src/bus/keyboard.cc
        src/bus/loader.cc
        src/bus/math_copro.cc
        src/bus/opl_2.cc
        src/bus/rtc.cc
        src/bus/system.cc
        src/bus/vicky.cc
        src/cpu/binary.cc
        )
set(BUS_HEADERS
        src/bus/automation.h
        src/bus/ch376_sd.h
        src/bus/int_controller.h
        src/bus/keyboard.h
        src/bus/loader.h
        src/bus/math_copro.h
        src/bus/opl_2.h
        src/bus/rtc.h
        src/bus/sdl_to_atset_keymap.h
        src/bus/system.h
        src/bus/vicky_def.h
        src/bus/vicky.h
        )
add_library(bus ${BUS_SOURCES} ${BUS_HEADERS})
add_dependencies(bus circular_buffer retro_cpu_65816 retro_host_linux retro_cpu_core)
target_include_directories(bus PUBLIC ${SDL2_INCLUDE_DIRS}
        ${GLOG_ROOT}/include ${SREC_INCLUDE_DIR}
        ${CIRCULAR_BUFFER_INCLUDE_DIRS}
        ${ADPLUG_INCLUDE_DIRS} ./src ./retro_cpu)
target_compile_options(bus PUBLIC ${SDL2_CFLAGS_OTHER} ${glog_CFLAGS_OTHER}
        -Werror -Wall -Wextra -Wno-unused-parameter)

# Main executable
add_executable(c256emu src/main.cc)
add_dependencies(c256emu bus gflags)
target_include_directories(c256emu PUBLIC
        ${CIRCULAR_BUFFER_INCLUDE_DIRS}
        ${LINENOISENG_INCLUDE_DIR}
        ${SREC_INCLUDE_DIR}
        ./src)
target_link_libraries(c256emu bus retro_cpu_65816 retro_cpu_65816 retro_host_linux retro_cpu_core pthread rt
        glog::glog
        gflags
        SDL2::SDL2main
        SDL2::SDL2
        Lua::lua_lib
        ${CIRCULAR_BUFFER_LIBRARIES}
        ${ADPLUG_LIBRARY}
        ${SREC_LIBRARIES}
        ${LINENOISENG_LIBRARY})
target_compile_options(c256emu PUBLIC -Werror -Wall -Wextra)

# Unit tests.
include(GoogleTest)
add_executable(c256_tests src/bus/math_copro_test.cc)
add_dependencies(c256_tests bus retro_cpu_core)
target_include_directories(c256_tests PUBLIC
        ${GTEST_INCLUDE_DIRS})
target_link_libraries(c256_tests bus retro_cpu_core
        glog::glog gflags GTest::main
        ${GTEST_MAIN_LIBRARY})
target_compile_options(c256_tests PUBLIC
        ${GLOG_CFLAGS_OTHER}
        ${GFLAGS_CFLAGS_OTHER} -Werror -Wall -Wextra)

gtest_add_tests(TARGET c256_tests)
