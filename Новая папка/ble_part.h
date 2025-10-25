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
    Реализовать HOGP, чтобы отображался уровень заряда
    Реализовать Extended Adverstiment

    По сути доступ из других мест должен быть только:
    1) К runtime-созданию новых сервисов
    2) Для работы c BAS (Лучше реализовать через поток). Изменение BatterLevel/BatterLevelStatus (Лучше реализовать через поток и очереди)
    3) Для работы c MAIN TASK (Лучше реализовать через поток). Изменение IN (Лучше реализовать через очереди). 
*/

extern struct bt_conn *ble_device;

int ble_begin();
int ble_create_main_task_service(bt_uuid_128 *uuid_service, bt_uuid_128 *uuid_characteristic_in, bt_uuid_128 *uuid_characteristic_out);

// DEBUG
void ble_test_notify(uint8_t new_out_value);
void ble_test_mtu();



#ifdef __cplusplus
}
#endif