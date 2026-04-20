project(apio16_cli C)

set(APP_NAME apio16_cli)

# Create executable
add_executable(${APP_NAME}
    apio16_cli.c
)

# Link against the user library
target_link_libraries(${APP_NAME}
    PRIVATE apio16
)

# Compute output path relative to repository root
get_repo_relative_path(REPO_PATH)
set(APP_OUT ${CMAKE_BINARY_DIR}/${REPO_PATH})
file(MAKE_DIRECTORY ${APP_OUT})

# Place the executable under the same folder structure in build/
set_target_properties(${APP_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${APP_OUT}
)