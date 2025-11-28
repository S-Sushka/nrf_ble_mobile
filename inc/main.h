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


#define NVS_ROOT_KEY_BT     "bt/"
#define NVS_ROOT_KEY_TIME   "time/"


#define NVS_KEY_BT_UUID_TRANSPORT_SERVICE              "uuid/transport/service"
#define NVS_KEY_BT_UUID_TRANSPORT_CHARACTERISTIC_IN    "uuid/transport/in"
#define NVS_KEY_BT_UUID_TRANSPORT_CHARACTERISTIC_OUT   "uuid/transport/out"

#define NVS_KEY_TIMEOUT_USB_RX_TIMEOUT   "usb_rx_timeout"


#define THREAD_STACK_SIZE_MAIN  2048
#define THREAD_PRIORITY_MAIN    0



void getFullSettingsPath(char *buf, const char *root, const char *key);