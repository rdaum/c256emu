cmake_minimum_required(VERSION 3.9)

# Enable Hunter package management.
include("cmake/HunterGate.cmake")
huntergate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.188.tar.gz"
        SHA1 "3bcf0667e771b05946d6a70b88a3e3a568b15452"
)

project(c256emu)

set(CMAKE_CXX_STANDARD 17)

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

hunter_add_package(imgui)
find_package(imgui CONFIG REQUIRED)

hunter_add_package(glfw)
find_package(glfw3 CONFIG REQUIRED)

# Add our local cmake dir.
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Configure circular_buffer a downloadable library.
# Configure circular_buffer a downloadable library.
include(cmake/External_circular_buffer.cmake)

# Add RetroCpu as an external library.
include(cmake/External_RetroCpu.cmake)

# The c256 bus devices
set(BUS_SOURCES
        src/automation/automation.cc
        src/automation/lua_describe.cc
        src/bus/ch376_sd.cc
        src/bus/int_controller.cc
        src/bus/keyboard.cc
        src/bus/loader.cc
        src/bus/math_copro.cc
        src/bus/rtc.cc
        src/bus/vicky.cc
        src/bus/c256_system_bus.cc)
set(BUS_HEADERS
        src/automation/automation.h
        src/automation/lua_describe.h
        src/bus/ch376_sd.h
        src/bus/int_controller.h
        src/bus/keyboard.h
        src/bus/loader.h
        src/bus/math_copro.h
        src/bus/rtc.h
        src/bus/sdl_to_atset_keymap.h
        src/bus/vicky_def.h
        src/bus/vicky.h
        src/bus/c256_system_bus.h
        )
add_library(bus ${BUS_SOURCES} ${BUS_HEADERS})
add_dependencies(bus circular_buffer retrocpu)
target_include_directories(bus PUBLIC ${SDL2_INCLUDE_DIRS}
        ${GLOG_ROOT}/include
        ${CIRCULAR_BUFFER_INCLUDE_DIRS}
        ${RETROCPU_INCLUDE_DIRS}
        ./src)
target_compile_options(bus PUBLIC ${SDL2_CFLAGS_OTHER} ${glog_CFLAGS_OTHER})

# Main executable
add_executable(c256emu
        src/main.cc
        src/system.cc
        src/gui/gui.cc
        src/gui/automation_console.cc src/gui/automation_console.h
        src/gui/imgui_impl_glfw.cpp src/gui/imgui_impl_glfw.h
        src/gui/imgui_impl_opengl2.cpp src/gui/imgui_impl_opengl2.h)
add_dependencies(c256emu bus gflags retrocpu circular_buffer)
target_include_directories(c256emu PUBLIC
        ${CIRCULAR_BUFFER_INCLUDE_DIRS}
        ${RETROCPU_INCLUDE_DIRS}
        ./src)

if (WIN32)
    set(PLATFORM_LIBRARIES})
else ()
    set(PLATFORM_LIBRARIES pthread rt GL)
endif ()

target_link_libraries(c256emu
        bus
        ${PLATFORM_LIBRARIES}
        glog::glog
        gflags
        SDL2::SDL2main
        SDL2::SDL2
        glfw
        imgui::imgui
        ${RETRO_CPU_CORE_LIBRARY_PATH}
        ${RETRO_CPU_65816_LIBRARY_PATH}
        ${RETRO_HOST_LIBRARY_PATH}
        Lua::lua_lib)


# Unit tests.
include(GoogleTest)
add_executable(c256_tests src/bus/math_copro_test.cc)
add_dependencies(c256_tests bus retrocpu)
target_include_directories(c256_tests PUBLIC
        ${GTEST_INCLUDE_DIRS}
        ${RETROCPU_INCLUDE_DIRS})
target_link_libraries(c256_tests bus
        glog::glog gflags GTest::main
        ${GTEST_MAIN_LIBRARY}
        ${RETRO_CPU_CORE_LIBRARY_PATH}
        ${RETRO_CPU_65816_LIBRARY_PATH}
        )
target_compile_options(c256_tests PUBLIC
        ${GLOG_CFLAGS_OTHER})

gtest_add_tests(TARGET c256_tests)
