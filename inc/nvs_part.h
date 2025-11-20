#pragma once

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <string.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#include <zephyr/bluetooth/uuid.h>


#ifdef __cplusplus
extern "C" {
#endif



#define STORAGE_PARTITION storage

int nvs_begin();
void nvs_test(struct bt_uuid_128 *uuid);

// Стек UUID динамических BLE-сервисов
uint8_t get_uuids_count();
int get_uuid(uint8_t index, struct bt_uuid_128 *uuid);
int add_uuid(struct bt_uuid_128 *uuid);
int delete_last_uuid();


#ifdef __cplusplus
}
#endif