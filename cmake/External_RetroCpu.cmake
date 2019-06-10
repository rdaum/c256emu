include(ExternalProject)
ExternalProject_Add(
        retrocpu
        GIT_REPOSITORY https://github.com/achaulk/retro_cpu.git
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)

ExternalProject_Get_Property(retrocpu source_dir)
set(RETROCPU_INCLUDE_DIRS ${source_dir}/)

# The cloning of the above repo doesn't happen until make, however if the dir doesn't
# exist, INTERFACE_INCLUDE_DIRECTORIES will throw an error.
# To make it work, we just create the directory now during config.
file(MAKE_DIRECTORY ${RETROCPU_INCLUDE_DIRS})

ExternalProject_Get_Property(retrocpu binary_dir)
set(RETROCPU_LIBRARY_PATH
        ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}retro_cpu_core.a
        ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}retro_cpu_65816.a
        ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}retro_host_linux.a
        )
set(RETROCPU_LIBRARY RETROCPU)
add_library(${RETROCPU_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${RETROCPU_LIBRARY} PROPERTIES
        "IMPORTED_LOCATION" "${RETROCPU_LIBRARY_PATH}"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${RETROCPU_INCLUDE_DIRS}")
add_dependencies(${RETROCPU_LIBRARY} retrocpu)
