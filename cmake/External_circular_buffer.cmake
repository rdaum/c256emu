include(ExternalProject)
ExternalProject_Add(
        circular_buffer
        GIT_REPOSITORY https://github.com/JustasMasiulis/circular_buffer.git
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON)

ExternalProject_Get_Property(circular_buffer source_dir)
set(CIRCULAR_BUFFER_INCLUDE_DIRS ${source_dir}/include)

# The cloning of the above repo doesn't happen until make, however if the dir doesn't
# exist, INTERFACE_INCLUDE_DIRECTORIES will throw an error.
# To make it work, we just create the directory now during config.
file(MAKE_DIRECTORY ${CIRCULAR_BUFFER_INCLUDE_DIRS})

