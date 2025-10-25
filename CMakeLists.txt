cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(nrf_ble_mobile)

target_include_directories(app PRIVATE inc)
target_compile_options(app PRIVATE -fno-threadsafe-statics)

target_sources(app PRIVATE
    src/nvs_part.cpp
    src/ble_part.cpp
    src/main.cpp
)
