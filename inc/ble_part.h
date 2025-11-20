#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/bas.h>


extern struct bt_conn *ble_device;

int ble_begin(bt_uuid_128 *transport_service_uuid, 
            bt_uuid_128 *transport_characteristic_in_uuid, 
            bt_uuid_128 *transport_characteristic_out_uuid);

// Обёртки - BAS
uint8_t ble_bas_get_battery_level();
uint8_t ble_bas_get_battery_level_status();

int ble_bas_set_battery_level(uint8_t value);
int ble_bas_set_battery_level_status(uint8_t value);


#ifdef __cplusplus
}
#endif