/*
 * main.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "main.h"
#include "ble_part.h"
#include "usb_part.h"

/*
	TODO:
	1) BLE. Добавить и потестить возможность подключения нескольких устройств
	2) Реализовать тестовый пример для всего этого дела из текста задания - https://jira.svc.uhfs.ru/browse/UW-9
*/



static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);


extern struct k_msgq parser_queue;  // Очередь парсера

struct bt_uuid_128 UUID_TRANSPORT_SERVICE;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_IN;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_OUT;



void _debug_print_uuid(struct bt_uuid_128 *uuid) 
{
    for (int i = 0; i < 16; i++)
        printk("%x", uuid->val[i]);
    printk("\n");
}


static int settings_bt_load(const char *key, size_t len,
                            settings_read_cb read_cb, void *cb_arg)
{
    if (strcmp(key, "uuid/transport/service") == 0) 
	{
        if (read_cb(cb_arg, &UUID_TRANSPORT_SERVICE, sizeof(struct bt_uuid_128)) == sizeof(struct bt_uuid_128)) 
			_debug_print_uuid(&UUID_TRANSPORT_SERVICE);		
    }

    if (strcmp(key, "uuid/transport/in") == 0) 
	{
        if (read_cb(cb_arg, &UUID_TRANSPORT_CHARACTERISTIC_IN, sizeof(struct bt_uuid_128)) == sizeof(struct bt_uuid_128)) 
			_debug_print_uuid(&UUID_TRANSPORT_CHARACTERISTIC_IN);		
    }
	
    if (strcmp(key, "uuid/transport/out") == 0) 
	{
        if (read_cb(cb_arg, &UUID_TRANSPORT_CHARACTERISTIC_OUT, sizeof(struct bt_uuid_128)) == sizeof(struct bt_uuid_128)) 
			_debug_print_uuid(&UUID_TRANSPORT_CHARACTERISTIC_OUT);		
    }	

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(settings_bt_handler, "bt",
                               NULL, settings_bt_load, NULL, NULL);


// *******************************************************************
// >>> MAIN <<<
// *******************************************************************

void main_thread(void)
{	
	gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT);
	gpio_pin_set_dt(&led0_dev, 1);


	settings_subsys_init();

	// Сохраняем UUID, если нужно
	struct bt_uuid_128 uuids[3] = 
	{
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E0, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E1, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E2, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
	};

	settings_save_one("bt/uuid/transport/service", 	&uuids[0], sizeof(struct bt_uuid_128));
	settings_save_one("bt/uuid/transport/in", 		&uuids[1], sizeof(struct bt_uuid_128));
	settings_save_one("bt/uuid/transport/out",	 	&uuids[2], sizeof(struct bt_uuid_128));

	settings_load();


	ble_begin(&UUID_TRANSPORT_SERVICE, &UUID_TRANSPORT_CHARACTERISTIC_IN, &UUID_TRANSPORT_CHARACTERISTIC_OUT);
	usb_begin();

	
	
	// uint8_t battery_level = 0;
	// uint8_t battery_level_status = 0;

    while (1) 
    {
		k_msleep(1000);
		// if (battery_level >= 100)
		// 	battery_level = 0;
		// else
		// 	battery_level += 10;
		// ble_bas_set_battery_level(battery_level);

		// if (battery_level_status >= 0x0F)
		// 	battery_level_status = 0;
		// else
		// 	battery_level_status++;
		// ble_bas_set_battery_level_status(battery_level_status);	
    }   	
}

K_THREAD_DEFINE(main_thread_id, 1024, main_thread, NULL, NULL, NULL,
		0, 0, 0);
