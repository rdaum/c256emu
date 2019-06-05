cmake_minimum_required(VERSION 3.10)
project(c256emu)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# External dependencies
find_package(PkgConfig)
find_package(Lua REQUIRED)
find_package(gflags REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLOG REQUIRED libglog)
pkg_check_modules(ADPLUG REQUIRED adplug)
pkg_check_modules(SREC REQUIRED srecord)

# Configure google-test as a downloadable library.
include(cmake/External_GTest.cmake)

# Configure liblinenoise-ng as a downloadable library.
include(cmake/External_LinenoiseNG.cmake)

# Configure circular_buffer a downloadable library.
include(cmake/External_circular_buffer.cmake)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

# CPU emulator. A fork of lib65816 from Francesco Rigoni.
file(GLOB CPU65816_SOURCES src/cpu/*.cc src/cpu/opcodes/*.cc)
file(GLOB CPU65816_HEADERS src/cpu/*.h src/cpu/opcodes/*.h)
add_library(cpu65816 ${CPU65816_SOURCES} ${CPU65816_HEADERS})
target_include_directories(cpu65816 PUBLIC ${GLOG_INCLUDE_DIRS} ./src)
target_compile_options(cpu65816 PUBLIC ${GLOG_CFLAGS_OTHER} -Werror -Wall)

# The c256 bus devices
file(GLOB BUS_SOURCES src/bus/*.cc)
file(GLOB BUS_HEADERS src/bus/*.h)
add_library(bus ${BUS_SOURCES} ${BUS_HEADERS})
add_dependencies(bus cpu65816 circular_buffer)
target_include_directories(bus PUBLIC ${SDL2_INCLUDE_DIRS} ${GLOG_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS} ${SREC_INCLUDE_DIR}
                            ${CIRCULAR_BUFFER_INCLUDE_DIRS}
                           ${ADPLUG_INCLUDE_DIRS} ./src)
target_compile_options(bus PUBLIC ${SDL2_CFLAGS_OTHER} ${GLOG_CFLAGS_OTHER} -Werror -Wall -Wextra -Wno-unused-parameter)

# Main executable
add_executable(c256emu src/main.cc)
add_dependencies(c256emu cpu65816 bus )
target_include_directories(c256emu PUBLIC
                           ${SDL2_INCLUDE_DIRS}
                           ${GLOG_INCLUDE_DIRS}
                           ${CIRCULAR_BUFFER_INCLUDE_DIRS}
                           ${LUA_INCLUDE_DIRS}
                           ${GFLAGS_INCLUDE_DIRS}
                           ${LINENOISENG_INCLUDE_DIR}
                           ${SREC_INCLUDE_DIR}
                           ./src)
target_link_libraries(c256emu bus cpu65816 pthread rt
                      ${SDL2_LIBRARIES}
                      ${GLOG_LIBRARIES}
                      ${CIRCULAR_BUFFER_LIBRARIES}
                      ${LUA_LIBRARIES}
                      ${GFLAGS_LIBRARIES}
                      ${ADPLUG_LIBRARY}
                      ${SREC_LIBRARIES}
                      ${LINENOISENG_LIBRARY})
target_compile_options(c256emu PUBLIC
                       ${SDL2_CFLAGS_OTHER}
                       ${GLOG_CFLAGS_OTHER}
                       ${GFLAGS_CFLAGS_OTHER} -Werror -Wall -Wextra)

# Unit tests.
include(GoogleTest)
add_executable(c256_tests src/bus/ram_device_test.cc src/bus/math_copro_test.cc)
add_dependencies(c256_tests cpu65816 bus googletest)
target_link_libraries(c256_tests bus cpu65816
                      ${GLOG_LIBRARIES}
                      ${GTEST_LIBRARY}
                      ${GTEST_MAIN_LIBRARY})
target_compile_options(c256_tests PUBLIC
                       ${GLOG_CFLAGS_OTHER}
                       ${GFLAGS_CFLAGS_OTHER} -Werror -Wall -Wextra)

gtest_add_tests(TARGET c256_tests)
