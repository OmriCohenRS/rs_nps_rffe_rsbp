cmake_minimum_required(VERSION 3.20)

project(apio16_userlib C)

set(LIB_NAME apio16)

# Collect source files
file(GLOB_RECURSE LIB_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Collect header files
set(LIB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

# Create shared library (.so)
add_library(${LIB_NAME} SHARED ${LIB_SOURCES})
target_include_directories(${LIB_NAME} PUBLIC ${LIB_INCLUDE_DIR})
target_link_libraries(${LIB_NAME} PUBLIC gpiod)

get_repo_relative_path(REPO_PATH)
set(LIB_OUT ${CMAKE_BINARY_DIR}/${REPO_PATH})
file(MAKE_DIRECTORY ${LIB_OUT})

set_target_properties(${LIB_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${LIB_OUT}
    ARCHIVE_OUTPUT_DIRECTORY ${LIB_OUT}
)

