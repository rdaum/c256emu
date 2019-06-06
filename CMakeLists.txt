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

# CPU emulator. A fork of lib65816 from Francesco Rigoni.
set(CPU65816_SOURCES
        src/cpu/addressing.cc
        src/cpu/binary.cc
        src/cpu/cpu_65816.cc
        src/cpu/cpu_status.cc
        src/cpu/stack.cc
        src/cpu/system_bus.cc
        src/cpu/system_bus_device.cc
        src/cpu/opcodes/OpCode_ADC.cc
        src/cpu/opcodes/OpCode_AND.cc
        src/cpu/opcodes/OpCode_ASL.cc
        src/cpu/opcodes/OpCode_BIT.cc
        src/cpu/opcodes/OpCode_Branch.cc
        src/cpu/opcodes/OpCode_CMP.cc
        src/cpu/opcodes/OpCode_CPX_CPY.cc
        src/cpu/opcodes/OpCode_EOR.cc
        src/cpu/opcodes/OpCode_INC_DEC.cc
        src/cpu/opcodes/OpCode_Interrupt.cc
        src/cpu/opcodes/OpCode_JumpReturn.cc
        src/cpu/opcodes/OpCode_LDA.cc
        src/cpu/opcodes/OpCode_LDX.cc
        src/cpu/opcodes/OpCode_LDY.cc
        src/cpu/opcodes/OpCode_LSR.cc
        src/cpu/opcodes/OpCode_Misc.cc
        src/cpu/opcodes/OpCode_ORA.cc
        src/cpu/opcodes/OpCode_ROL.cc
        src/cpu/opcodes/OpCode_ROR.cc
        src/cpu/opcodes/OpCode_SBC.cc
        src/cpu/opcodes/OpCode_STA.cc
        src/cpu/opcodes/OpCode_Stack.cc
        src/cpu/opcodes/OpCode_StatusReg.cc
        src/cpu/opcodes/OpCode_STX.cc
        src/cpu/opcodes/OpCode_STY.cc
        src/cpu/opcodes/OpCode_STZ.cc
        src/cpu/opcodes/OpCodeTable.cc
        src/cpu/opcodes/OpCode_Transfer.cc
        src/cpu/opcodes/OpCode_TSB_TRB.cc
        )
set(CPU65816_HEADERS
        src/cpu/binary.h
        src/cpu/cpu_65816.h
        src/cpu/cpu_status.h
        src/cpu/interrupt.h
        src/cpu/opcode.h
        src/cpu/stack.h
        src/cpu/system_bus_device.h
        src/cpu/system_bus.h
        )
add_library(cpu65816 ${CPU65816_SOURCES} ${CPU65816_HEADERS})
target_include_directories(cpu65816 PUBLIC ${GLOG_ROOT}/include ./src)
target_compile_options(cpu65816 PUBLIC ${GLOG_CFLAGS_OTHER} -Werror -Wall)

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
        src/bus/ram_device.cc
        src/bus/rtc.cc
        src/bus/system.cc
        src/bus/vicky.cc

        )
set(BUS_HEADERS
        src/bus/automation.h
        src/bus/ch376_sd.h
        src/bus/int_controller.h
        src/bus/keyboard.h
        src/bus/loader.h
        src/bus/math_copro.h
        src/bus/opl_2.h
        src/bus/ram_device.h
        src/bus/rtc.h
        src/bus/sdl_to_atset_keymap.h
        src/bus/system.h
        src/bus/vicky_def.h
        src/bus/vicky.h
        )
add_library(bus ${BUS_SOURCES} ${BUS_HEADERS})
add_dependencies(bus cpu65816 circular_buffer)
target_include_directories(bus PUBLIC ${SDL2_INCLUDE_DIRS}
        ${GLOG_ROOT}/include ${SREC_INCLUDE_DIR}
        ${CIRCULAR_BUFFER_INCLUDE_DIRS}
        ${ADPLUG_INCLUDE_DIRS} ./src)
target_compile_options(bus PUBLIC ${SDL2_CFLAGS_OTHER} ${glog_CFLAGS_OTHER}
        -Werror -Wall -Wextra -Wno-unused-parameter)

# Main executable
add_executable(c256emu src/main.cc)
add_dependencies(c256emu cpu65816 bus gflags)
target_include_directories(c256emu PUBLIC
        ${CIRCULAR_BUFFER_INCLUDE_DIRS}
        ${LINENOISENG_INCLUDE_DIR}
        ${SREC_INCLUDE_DIR}
        ./src)
target_link_libraries(c256emu bus cpu65816 pthread rt
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
add_executable(c256_tests src/bus/ram_device_test.cc src/bus/math_copro_test.cc)
add_dependencies(c256_tests cpu65816 bus)
target_include_directories(c256_tests PUBLIC
        ${GTEST_INCLUDE_DIRS})
target_link_libraries(c256_tests bus cpu65816
        glog::glog gflags GTest::main
        ${GTEST_MAIN_LIBRARY})
target_compile_options(c256_tests PUBLIC
        ${GLOG_CFLAGS_OTHER}
        ${GFLAGS_CFLAGS_OTHER} -Werror -Wall -Wextra)

gtest_add_tests(TARGET c256_tests)
