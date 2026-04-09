cmake_minimum_required(VERSION 3.20)

# ---------------------------------------------------------
# Project Definition
# ---------------------------------------------------------
project(NPS_RFFE_RSBP
    VERSION 1.0
    LANGUAGES C CXX
)

# ---------------------------------------------------------
# Global Settings
# ---------------------------------------------------------
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# Compute a variable that always contains repo root
set(REPO_ROOT ${CMAKE_SOURCE_DIR})

function(get_repo_relative_path out_var)
    file(RELATIVE_PATH rel_path "${REPO_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
    set(${out_var} ${rel_path} PARENT_SCOPE)
endfunction()

# Where to put all compiled binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ---------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------
add_subdirectory(kernel/drivers)
add_subdirectory(device_tree/overlays)
add_subdirectory(user_space/lib)
add_subdirectory(user_space/cli)