include(ExternalProject)
ExternalProject_Add(
        retrocpu
        GIT_REPOSITORY https://github.com/rdaum/retro_cpu.git
        GIT_TAG patch-1
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        CMAKE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
    )


ExternalProject_Get_Property(retrocpu source_dir)
set(RETROCPU_INCLUDE_DIRS ${source_dir}/)

# The cloning of the above repo doesn't happen until make, however if the dir doesn't
# exist, INTERFACE_INCLUDE_DIRECTORIES will throw an error.
# To make it work, we just create the directory now during config.
file(MAKE_DIRECTORY ${RETROCPU_INCLUDE_DIRS})

ExternalProject_Get_Property(retrocpu binary_dir)

if(WIN32)
    set(RETRO_CPU_LIBRARY_SUFFIX .lib)
else()
    set(RETRO_CPU_LIBRARY_SUFFIX .a)
endif()

set(RETRO_CPU_CORE_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}retro_cpu_core${RETRO_CPU_LIBRARY_SUFFIX})
add_library(retro_cpu_core STATIC IMPORTED)
set_target_properties(retro_cpu_core PROPERTIES
        "IMPORTED_LOCATION" "${RETRO_CPU_CORE_LIBRARY_PATH}"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${RETROCPU_INCLUDE_DIRS}")
add_dependencies(retro_cpu_core retrocpu)

set(RETRO_CPU_65816_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}retro_cpu_65816${RETRO_CPU_LIBRARY_SUFFIX})
add_library(retro_cpu_65816 STATIC IMPORTED)
set_target_properties(retro_cpu_65816 PROPERTIES
        "IMPORTED_LOCATION" "${RETRO_CPU_65816_LIBRARY_PATH}"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${RETROCPU_INCLUDE_DIRS}")
add_dependencies(retro_cpu_65816 retrocpu)


set(RETRO_HOST_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}retro_host${RETRO_CPU_LIBRARY_SUFFIX})
add_library(retro_host STATIC IMPORTED)
set_target_properties(retro_host PROPERTIES
        "IMPORTED_LOCATION" "${RETRO_HOST_LIBRARY_PATH}"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${RETROCPU_INCLUDE_DIRS}")
add_dependencies(retro_host retrocpu)

