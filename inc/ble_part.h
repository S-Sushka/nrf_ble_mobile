#pragma once

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/services/bas.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
    TODO: 
    Переписать это всё дело под Extended Avertisement
    Добавить поддержку HoG, хотя бы просто чтобы оно подключалось
    Доделать динамическое выделение сервисов через односвязные списки
*/

extern struct bt_conn *ble_device;

int ble_begin();
int ble_add_service(bt_uuid_128 *service_uuid, bt_gatt_attr *attributes, size_t attributes_len);




#ifdef __cplusplus
}
#endif