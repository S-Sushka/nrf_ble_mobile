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
    При отключении всё падает, переподключиться нельзя, только сбрасывать. Почини
    Реализовать HOGP, чтобы отображался уровень заряда
    Реализовать Extended Adverstiment (Стоит ли?)



    По сути доступ из других мест должен быть только:
    2) Для работы c BAS (Лучше реализовать через поток). Изменение BatterLevel/BatterLevelStatus (Лучше реализовать через поток и очереди)
    3) Для работы c MAIN TASK (Лучше реализовать через поток). Изменение IN (Лучше реализовать через очереди). 
*/

extern struct bt_conn *ble_device;

int ble_begin();
int ble_add_service(bt_uuid_128 *service_uuid, bt_gatt_attr *attributes, size_t attributes_len);




#ifdef __cplusplus
}
#endif