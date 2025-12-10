/*
 * main.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

 
#include "main.h"


#include "nvs_part.h"
#include "parser.h"
#include "ble_part.h"
#include "usb_part.h"


int settings_init_load();

static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);


struct bt_uuid_128 UUID_TRANSPORT_SERVICE;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_IN;
struct bt_uuid_128 UUID_TRANSPORT_CHARACTERISTIC_OUT;

uint16_t USB_RX_TIMEOUT_VALUE;
uint16_t BLE_RX_TIMEOUT_VALUE;



#include <psa/crypto.h>
#include <psa/crypto_extra.h>

void printKCV(uint8_t *kcv, uint16_t len)
{
	SEGGER_RTT_printf(0, "KCV: ");
	for (int i = 0; i < len; i++) 
	{
		if (kcv[i] < 0x10)
			SEGGER_RTT_printf(0, "0");
		SEGGER_RTT_printf(0, "%x", kcv[i]);
	}
	SEGGER_RTT_printf(0, "\n");
}

static psa_key_id_t key_id;
int crypto_init(void)
{
	if (psa_crypto_init() != PSA_SUCCESS)
	{
		SEGGER_RTT_printf(0, "PSA Init Failed!\n");
		return -1;
	}
	return 0;
}

int import_key(void)
{
	psa_status_t status;

	uint8_t key_val[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	psa_key_attributes_t key_attributes = PSA_KEY_ATTRIBUTES_INIT;


	psa_set_key_usage_flags(&key_attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_lifetime(&key_attributes, PSA_KEY_LIFETIME_VOLATILE);
	psa_set_key_algorithm(&key_attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&key_attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&key_attributes, 128);

	
	status = psa_import_key(&key_attributes, key_val, sizeof(key_val), &key_id);
	if (status != PSA_SUCCESS) 
	{
		SEGGER_RTT_printf(0, "Key Import Failed: %d\n", status);
		return -1;
	}	

	psa_reset_key_attributes(&key_attributes);
	return 0;
}

int encrypt_cbc_aes(void)
{
	uint32_t olen;
	psa_status_t status;
	psa_cipher_operation_t operation = PSA_CIPHER_OPERATION_INIT;

	uint8_t data_in_val[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	uint8_t data_out_val[sizeof(data_in_val)];

	status = psa_cipher_encrypt_setup(&operation, key_id, PSA_ALG_ECB_NO_PADDING);
	if (status != PSA_SUCCESS) 
	{
		SEGGER_RTT_printf(0, "psa_cipher_encrypt_setup failed! (Error: %d)\n", status);
		return -1;
	}

	status = psa_cipher_update(&operation, data_in_val, sizeof(data_in_val), 
						data_out_val, sizeof(data_out_val), 
						&olen);
	if (status != PSA_SUCCESS) {
		SEGGER_RTT_printf(0, "psa_cipher_update failed! (Error: %d)\n", status);
		return -1;
	}

	status = psa_cipher_finish(&operation, data_out_val + olen,
				   sizeof(data_out_val) - olen,
				   &olen);
	if (status != PSA_SUCCESS) 
	{
		SEGGER_RTT_printf(0, "psa_cipher_finish failed! (Error: %d)\n", status);
		return -1;
	}

	printKCV(data_out_val, 4);
	psa_cipher_abort(&operation);

	return 0;
}


// *******************************************************************
// >>> MAIN <<<
// *******************************************************************

void incrementTestNotifyData(uint8_t *notify_data);
void main_thread(void)
{	
	int err = 0;

	gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT);
	gpio_pin_set_dt(&led0_dev, 1);

	crypto_init();
	import_key();
	encrypt_cbc_aes();

	while (1) 
	{
		k_msleep(10000);
	}


	settings_subsys_init();
	SEGGER_RTT_printf(0, "Save Res: %d\n", settings_init_save());

	user_data_load_uuid_service(&UUID_TRANSPORT_SERVICE);
	user_data_load_uuid_characteristic_in(&UUID_TRANSPORT_CHARACTERISTIC_IN);
	user_data_load_uuid_characteristic_out(&UUID_TRANSPORT_CHARACTERISTIC_OUT);

	user_data_load_time_usb_rx_timeout(&USB_RX_TIMEOUT_VALUE);
	user_data_load_time_ble_rx_timeout(&BLE_RX_TIMEOUT_VALUE);	


	ble_begin(BLE_RX_TIMEOUT_VALUE, &UUID_TRANSPORT_SERVICE, &UUID_TRANSPORT_CHARACTERISTIC_IN, &UUID_TRANSPORT_CHARACTERISTIC_OUT);
	usb_begin(USB_RX_TIMEOUT_VALUE);
	
	

	uint8_t battery_level = 0;
	uint8_t battery_level_status = 0;


	tUniversalMessage *notify_packet_usb;
   	tUniversalMessage *notify_packet_ble;
	uint8_t notify_data[9];

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
		incrementTestNotifyData(notify_data);
		
		// Notify Test USB
		notify_packet_usb = k_heap_alloc(&UniversalTransportHeap, sizeof(tUniversalMessage), K_NO_WAIT);
		notify_packet_usb->length = 9;
		notify_packet_usb->data = k_heap_alloc(&UniversalTransportHeap, notify_packet_usb->length, K_NO_WAIT); 
		for (int i = 0; i < notify_packet_usb->length; i++)
			notify_packet_usb->data[i] = notify_data[i];

		notify_packet_usb->source = MESSAGE_SOURCE_USB;
		err = k_msgq_put(&usb_queue_tx, &notify_packet_usb, K_NO_WAIT);
        if (err != 0) SEGGER_RTT_printf(0, " --- USB TX ERR --- :  USB TX Queue put error: %d\n", err);

		// Notify Test BLE
		for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
		{
			notify_packet_ble = k_heap_alloc(&UniversalTransportHeap, sizeof(tUniversalMessage), K_NO_WAIT);
			notify_packet_ble->length = 9;
			notify_packet_ble->data = k_heap_alloc(&UniversalTransportHeap, notify_packet_ble->length, K_NO_WAIT); 
			for (int i = 0; i < notify_packet_ble->length; i++)
				notify_packet_ble->data[i] = notify_data[i];

			notify_packet_ble->source = MESSAGE_SOURCE_BLE_CONNS + i;
			err = k_msgq_put(&ble_queue_tx, &notify_packet_ble, K_NO_WAIT);
        	if (err != 0) SEGGER_RTT_printf(0, " --- BLE TX ERR --- : BLE TX Queue put error: %d\n", err);
		}
		

		k_msleep(10000);
    }   	
}

K_THREAD_DEFINE(main_thread_id, THREAD_STACK_SIZE_MAIN, main_thread, NULL, NULL, NULL,
		THREAD_PRIORITY_MAIN, 0, 0);



void incrementTestNotifyData(uint8_t *notify_data) 
{
	static uint8_t TestNotifyValue = 255;
	TestNotifyValue++;

	notify_data[PROTOCOL_INDEX_PREAMBLE] 	= PROTOCOL_PREAMBLE;
	notify_data[PROTOCOL_INDEX_MSG_TYPE] 			= PROTOCOL_MSG_TYPE_PR_NOTIFY;
	notify_data[PROTOCOL_INDEX_MSG_CODE] 			= 0;

	notify_data[PROTOCOL_INDEX_PL_LEN_MSB] 		= 0;
	notify_data[PROTOCOL_INDEX_PL_LEN_LSB] 	= 1;

	notify_data[PROTOCOL_INDEX_PL_START] 	= TestNotifyValue;

	notify_data[PROTOCOL_INDEX_PL_START+1] 	= PROTOCOL_END_MARK;

	uint16_t crcCalculatedBuf = calculateCRC(notify_data, PROTOCOL_INDEX_PL_START+2);
	notify_data[PROTOCOL_INDEX_PL_START+2] 	= crcCalculatedBuf & 0x00FF;
	crcCalculatedBuf >>= 8;
	notify_data[PROTOCOL_INDEX_PL_START+3] 	= crcCalculatedBuf & 0x00FF;
}