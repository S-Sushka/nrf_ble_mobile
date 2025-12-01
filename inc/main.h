/*
 * main.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/settings/settings.h>



#define NVS_PATH_FACTORY_DATA_FD_FLAGS                          "factory_data/FD_FLAGS"
#define NVS_PATH_FACTORY_DATA_FD_TAG_SN                         "factory_data/FD_TAG_SN"
#define NVS_PATH_FACTORY_DATA_FD_TAG_MODEL                      "factory_data/FD_TAG_MODEL"
#define NVS_PATH_FACTORY_DATA_FD_TAG_ANALOG_BOARD_ID            "factory_data/FD_TAG_ANALOG_BOARD_ID"
#define NVS_PATH_FACTORY_DATA_FD_TAG_SEED                       "factory_data/FD_TAG_SEED"
#define NVS_PATH_FACTORY_DATA_FD_TAG_ISSUER_PUBLIC_KEY          "factory_data/FD_TAG_ISSUER_PUBLIC_KEY"
#define NVS_PATH_FACTORY_DATA_FD_TAG_ISSUER_CERTIFICATE         "factory_data/FD_TAG_ISSUER_CERTIFICATE"
#define NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_CERTIFICATE         "factory_data/FD_TAG_DEVICE_CERTIFICATE"
#define NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_PRIVATE_KEY         "factory_data/FD_TAG_DEVICE_PRIVATE_KEY"
#define NVS_PATH_FACTORY_DATA_FD_TAG_IOT_CA_CERTIFICATE         "factory_data/FD_TAG_IOT_CA_CERTIFICATE"
#define NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_HUK_KCV             "factory_data/FD_TAG_DEVICE_HUK_KCV"
#define NVS_PATH_FACTORY_DATA_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY "factory_data/FD_TAG_PROFILES_ISSUER_PUBLIC_KEY"
#define NVS_PATH_FACTORY_DATA_FD_TAG_BASE_USER_SETTINGS         "factory_data/FD_TAG_BASE_USER_SETTINGS"
#define NVS_PATH_FACTORY_DATA_FD_TAG_DEFAULT_LISENCE            "factory_data/FD_TAG_DEFAULT_LISENCE"
#define NVS_PATH_FACTORY_DATA_FD_TAG_SIGNATURE                  "factory_data/FD_TAG_SIGNATURE"

#define NVS_PATH_BT_UUID_TRANSPORT_SERVICE              "bt/uuid/transport/service"
#define NVS_PATH_BT_UUID_TRANSPORT_CHARACTERISTIC_IN    "bt/uuid/transport/in"
#define NVS_PATH_BT_UUID_TRANSPORT_CHARACTERISTIC_OUT   "bt/uuid/transport/out"

#define NVS_PATH_TIME_USB_RX_TIMEOUT   "time/usb_rx_timeout"


#define THREAD_STACK_SIZE_MAIN  2048
#define THREAD_PRIORITY_MAIN    0



int settings_init_save(); // Сохраняем настройки, если нужно
int settings_init_load();