include(ExternalProject)
ExternalProject_Add(
        linenoise-ng
        GIT_REPOSITORY https://github.com/arangodb/linenoise-ng.git
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)

ExternalProject_Get_Property(linenoise-ng source_dir)
set(LINENOISENG_INCLUDE_DIRS ${source_dir}/include)

# The cloning of the above repo doesn't happen until make, however if the dir doesn't
# exist, INTERFACE_INCLUDE_DIRECTORIES will throw an error.
# To make it work, we just create the directory now during config.
file(MAKE_DIRECTORY ${LINENOISENG_INCLUDE_DIRS})

ExternalProject_Get_Property(linenoise-ng binary_dir)
set(LINENOISENG_LIBRARY_PATH ${binary_dir}/${CMAKE_FIND_LIBRARY_PREFIXES}linenoise.a)
set(LINENOISENG_LIBRARY LINENOISENG)
add_library(${LINENOISENG_LIBRARY} UNKNOWN IMPORTED)
set_target_properties(${LINENOISENG_LIBRARY} PROPERTIES
        "IMPORTED_LOCATION" "${LINENOISENG_LIBRARY_PATH}"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        "INTERFACE_INCLUDE_DIRECTORIES" "${LINENOISENG_INCLUDE_DIRS}")
add_dependencies(${LINENOISENG_LIBRARY} linenoise-ng)
