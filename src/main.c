#include "main.h"
#include "nvs_part.h"
#include "ble_part.h"
#include "usb_part.h"

/*
	TODO:
	1) BLE. Добавить и потестить возможность подключения нескольких устройств
	2) Реализовать тестовый пример для всего этого дела из текста задания - https://jira.svc.uhfs.ru/browse/UW-9

	Второстепенные моменты:
	1) Сохраение данных кучей в NVS - https://docs.zephyrproject.org/latest/doxygen/html/settings_8h.html
	2) Настройка VS Code для заголовков - https://conf.svc.uhfs.ru/confluence/pages/viewpage.action?spaceKey=DEVEL&title=Visual+Studio+Code
*/



static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

//#define NVS_NEED_CONFIGURE // <-- Загружает настройки в NVS: сохраняет сервисов/характеристик UUID

extern struct k_msgq parser_queue;  // Очередь парсера

struct bt_uuid_128 UUID_TRANSPORT_SERVICE;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_IN;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_OUT;


// *******************************************************************
// >>> MAIN <<<
// *******************************************************************

void main_thread(void)
{	
	gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT);
	gpio_pin_set_dt(&led0_dev, 1);


	nvs_begin();

#ifdef NVS_NEED_CONFIGURE
	struct bt_uuid_128 uuids[3] = 
	{
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E0, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E1, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E2, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),

	};

	add_uuid(&uuids[0]);
	add_uuid(&uuids[1]);
	add_uuid(&uuids[2]);
#endif

	get_uuid(0, &UUID_TRANSPORT_SERVICE);
	get_uuid(1, &UUID_TRANSPORT_CHARACTERISTIC_IN);
	get_uuid(2, &UUID_TRANSPORT_CHARACTERISTIC_OUT);
		
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
