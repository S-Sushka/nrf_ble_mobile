/*
 * main.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "main.h"

#include "parser.h"
#include "ble_part.h"
#include "usb_part.h"



static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);


struct bt_uuid_128 UUID_TRANSPORT_SERVICE;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_IN;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_OUT;

uint16_t USB_RX_TIMEOUT_VALUE;


void getFullSettingsPath(char *buf, const char *root, const char *key) 
{
	memcpy(buf, root, sizeof(root)-1);
	memcpy(&buf[sizeof(root)-1], key, sizeof(key)-1);	
}

static int settings_bt_load(const char *key, size_t len,
                            settings_read_cb read_cb, void *cb_arg)
{
	// UUIDs
    if (strcmp( key, NVS_KEY_BT_UUID_TRANSPORT_SERVICE ) == 0) 
	{
        if (read_cb(cb_arg, &UUID_TRANSPORT_SERVICE, sizeof(struct bt_uuid_128)) != sizeof(struct bt_uuid_128)) 
			printk(" --- BLE UUID ERR --- : Bad Service UUID!\n");
    }

    if (strcmp( key, NVS_KEY_BT_UUID_TRANSPORT_CHARACTERISTIC_IN ) == 0) 
	{
        if (read_cb(cb_arg, &UUID_TRANSPORT_CHARACTERISTIC_IN, sizeof(struct bt_uuid_128)) != sizeof(struct bt_uuid_128)) 
			printk(" --- BLE UUID ERR --- : Bad IN Characteristic UUID!\n");
    }
	
    if (strcmp( key, NVS_KEY_BT_UUID_TRANSPORT_CHARACTERISTIC_OUT ) == 0)
	{
        if (read_cb(cb_arg, &UUID_TRANSPORT_CHARACTERISTIC_OUT, sizeof(struct bt_uuid_128)) != sizeof(struct bt_uuid_128)) 
			printk(" --- BLE UUID ERR --- : Bad OUT Characteristic UUID!\n");
				
    }	

	// Time
    if (strcmp( key, NVS_KEY_TIMEOUT_USB_RX_TIMEOUT ) == 0) 
	{
        if (read_cb(cb_arg, &USB_RX_TIMEOUT_VALUE, sizeof(uint16_t)) != sizeof(uint16_t)) 
			printk(" --- USB ERR --- : Bad USB RX Timeout Value!\n");
				
    }		

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(settings_bt_handler, "bt",
                               NULL, settings_bt_load, NULL, NULL);



// *******************************************************************
// >>> MAIN <<<
// *******************************************************************

void incrementTestNotifyPacket(tUniversalMessageTX *notify_packet);
void main_thread(void)
{	
	gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT);
	gpio_pin_set_dt(&led0_dev, 1);


	settings_subsys_init();

	// Сохраняем настройки, если нужно
	char full_path[SETTINGS_MAX_VAL_LEN] = { 0 };

	struct bt_uuid_128 uuid_bufs[3] = 
	{
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E0, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E1, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E2, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
	};
	uint16_t usb_rx_timeout_buf = 150;

	
	getFullSettingsPath(full_path, NVS_ROOT_KEY_BT, NVS_KEY_BT_UUID_TRANSPORT_SERVICE);
	settings_save_one(full_path, &uuid_bufs[0], sizeof(struct bt_uuid_128));

	getFullSettingsPath(full_path, NVS_ROOT_KEY_BT, NVS_KEY_BT_UUID_TRANSPORT_CHARACTERISTIC_IN);
	settings_save_one(full_path, &uuid_bufs[1], sizeof(struct bt_uuid_128));

	getFullSettingsPath(full_path, NVS_ROOT_KEY_BT, NVS_KEY_BT_UUID_TRANSPORT_CHARACTERISTIC_OUT);
	settings_save_one(full_path, &uuid_bufs[2], sizeof(struct bt_uuid_128));

	getFullSettingsPath(full_path, NVS_ROOT_KEY_TIME, NVS_KEY_TIMEOUT_USB_RX_TIMEOUT);
	settings_save_one(full_path, &usb_rx_timeout_buf, sizeof(uint16_t));

	settings_load();


	ble_begin(&UUID_TRANSPORT_SERVICE, &UUID_TRANSPORT_CHARACTERISTIC_IN, &UUID_TRANSPORT_CHARACTERISTIC_OUT);
	usb_begin(USB_RX_TIMEOUT_VALUE);

	
	
	uint8_t battery_level = 0;
	uint8_t battery_level_status = 0;

	tUniversalMessageTX notify_packet;
	uint8_t notify_data[MESSAGE_BUFFER_SIZE];
	notify_packet.data = notify_data;

	int err = 0;

    while (1) 
    {
		// BAS
		if (battery_level >= 100)
			battery_level = 0;
		else
			battery_level += 10;
		ble_bas_set_battery_level(battery_level);

		if (battery_level_status >= 0x0F)
			battery_level_status = 0;
		else
			battery_level_status++;
		ble_bas_set_battery_level_status(battery_level_status);	

		// TRANSPORT
		incrementTestNotifyPacket(&notify_packet);
		
		notify_packet.source = MESSAGE_SOURCE_USB;
		err = k_msgq_put(&usb_queue_tx, &notify_packet, K_NO_WAIT);
        if (err != 0) printk(" --- USB TX ERR --- :  USB TX Queue put error: %d\n", err);

		for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
		{
			notify_packet.source = MESSAGE_SOURCE_BLE_CONNS + i;
			err = k_msgq_put(&ble_queue_tx, &notify_packet, K_NO_WAIT);
        	if (err != 0) printk(" --- BLE TX ERR --- : BLE TX Queue put error: %d\n", err);
		}

		k_msleep(10000);
    }   	
}

K_THREAD_DEFINE(main_thread_id, THREAD_STACK_SIZE_MAIN, main_thread, NULL, NULL, NULL,
		THREAD_PRIORITY_MAIN, 0, 0);

		

void incrementTestNotifyPacket(tUniversalMessageTX *notify_packet) 
{
	static uint8_t TestNotifyValue = 255;
	TestNotifyValue++;

	notify_packet->data[PROTOCOL_INDEX_PREAMBLE] 	= PROTOCOL_PREAMBLE;
	notify_packet->data[PROTOCOL_INDEX_MT] 			= PROTOCOL_MSG_TYPE_PR_NOTIFY;
	notify_packet->data[PROTOCOL_INDEX_MC] 			= 0;

	notify_packet->data[PROTOCOL_INDEX_PL_LEN] 		= 0;
	notify_packet->data[PROTOCOL_INDEX_PL_LEN+1] 	= 1;

	notify_packet->data[PROTOCOL_INDEX_PL_START] 	= TestNotifyValue;

	notify_packet->data[PROTOCOL_INDEX_PL_START+1] 	= PROTOCOL_END_MARK;

	uint16_t crcCalculatedBuf = calculateCRC(notify_packet->data, PROTOCOL_INDEX_PL_START+2);
	notify_packet->data[PROTOCOL_INDEX_PL_START+2] 	= crcCalculatedBuf & 0x00FF;
	crcCalculatedBuf >>= 8;
	notify_packet->data[PROTOCOL_INDEX_PL_START+3] 	= crcCalculatedBuf & 0x00FF;

	notify_packet->length = 9;
}