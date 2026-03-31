cmake_minimum_required(VERSION 3.10)
project(apio16 C)

set(MODULE_NAME apio16)
set(MODULE_SOURCES apio16.c)

# Output directory for final .ko file
set(MODULE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/kernel/drivers/${MODULE_NAME})
file(MAKE_DIRECTORY ${MODULE_OUTPUT_DIR})

# Kernel build directory
set(KERNEL_BUILD_DIR /lib/modules/${CMAKE_SYSTEM_VERSION}/build)

# Module source tree (Kbuild MUST build here)
set(MODULE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MODULE_KO ${MODULE_OUTPUT_DIR}/${MODULE_NAME}.ko)


add_custom_command(
    OUTPUT ${MODULE_KO}

    # 1. Build module
    COMMAND ${CMAKE_MAKE_PROGRAM}
        -C ${KERNEL_BUILD_DIR}
        M=${MODULE_SOURCE_DIR}
        modules

    # 2. Copy .ko to build directory
    COMMAND ${CMAKE_COMMAND} -E copy
        ${MODULE_SOURCE_DIR}/${MODULE_NAME}.ko
        ${MODULE_KO}

    DEPENDS ${MODULE_SOURCES}
)

add_custom_target(${MODULE_NAME}_module ALL DEPENDS ${MODULE_KO})