cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf_ble_mobile)

target_include_directories(app PRIVATE inc)

target_sources(app PRIVATE
    src/PROTOCOL.c
    src/parser.c
    src/nvs_part.c
    src/ble_part.c
    src/usb_part.c
    src/main.c
)
