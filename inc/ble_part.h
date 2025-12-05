/*
 * ble_part.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#pragma once

#include <SEGGER_RTT.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/bas.h>

#include "PROTOCOL.h"



#define THREAD_STACK_SIZE_BLE   16384
#define THREAD_PRIORITY_BLE     7


extern struct k_msgq ble_queue_tx;  // Очередь отправки пакетов по BLE

int ble_begin(uint16_t rx_timeout_ms, 
            struct bt_uuid_128 *transport_service_uuid, 
            struct bt_uuid_128 *transport_characteristic_in_uuid, 
            struct bt_uuid_128 *transport_characteristic_out_uuid);


// Обёртки - BAS
uint8_t ble_bas_get_battery_level();
uint8_t ble_bas_get_battery_level_status();

void ble_bas_set_battery_level(uint8_t value);
void ble_bas_set_battery_level_status(uint8_t value);