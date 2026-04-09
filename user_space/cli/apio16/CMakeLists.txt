project(apio16_cli C)

# Collect all C files in this directory
file(GLOB CLI_SOURCES "*.c")

foreach(SRC ${CLI_SOURCES})
    get_filename_component(APP_NAME ${SRC} NAME_WE)

    add_executable(${APP_NAME} ${SRC})

    # Link to user-space library
    target_link_libraries(${APP_NAME} PRIVATE apio16)

    # Include the apio16 user lib headers
    target_include_directories(${APP_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/user_space/lib/apio16/include
    )

    get_repo_relative_path(REPO_PATH)
    set(CLI_OUT ${CMAKE_BINARY_DIR}/${REPO_PATH})
    file(MAKE_DIRECTORY ${CLI_OUT})

    set_target_properties(${APP_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CLI_OUT}
    )

endforeach()