# SPDX-License-Identifier: Apache-2.0
#
# RP2350B SoC build glue (Zephyr v4.1 compatibility shim).
# Reuses upstream rp2350 sources, linker script, and common Pico SoC code.

add_subdirectory(${ZEPHYR_BASE}/soc/raspberrypi/rpi_pico/common
                 ${CMAKE_CURRENT_BINARY_DIR}/rpi_pico_common)

add_subdirectory(${ZEPHYR_BASE}/soc/raspberrypi/rpi_pico/rp2350
                 ${CMAKE_CURRENT_BINARY_DIR}/rp2350)
