cmake_minimum_required(VERSION 3.20)

project(device_tree_overlays C)

#
# --- Find system DTC (correct on Raspberry Pi) ---
#
# Raspberry Pi kernel headers do NOT include dtc in:
#   /lib/modules/<ver>/build/scripts/dtc/dtc
# So we must use the system dtc tool.
#
find_program(DTC_EXECUTABLE dtc REQUIRED)

#
# --- Output directory for built overlays ---
#
get_repo_relative_path(REPO_PATH)
set(OVERLAY_OUT ${CMAKE_BINARY_DIR}/${REPO_PATH})
file(MAKE_DIRECTORY ${OVERLAY_OUT})

#
# --- Find all .dts files in the folder ---
#
file(GLOB DTS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.dts")

#
# --- Build rule for every .dts file ---
#
foreach(DTS ${DTS_FILES})
    get_filename_component(DTS_NAME ${DTS} NAME_WE)

    set(DTBO_FILE "${OVERLAY_OUT}/${DTS_NAME}.dtbo")

    add_custom_command(
        OUTPUT ${DTBO_FILE}
        COMMAND ${DTC_EXECUTABLE}
            -@                     # enable symbols
            -I dts                 # input format
            -O dtb                 # output format
            -o ${DTBO_FILE}        # output file
            ${DTS}
        DEPENDS ${DTS}
        COMMENT "Compiling device tree overlay: ${DTS_NAME}.dtbo"
        VERBATIM
    )

    add_custom_target(${DTS_NAME}_dtbo ALL
        DEPENDS ${DTBO_FILE}
    )
endforeach()