/*
 * nvs_part.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

#include "nvs_part.h"



// *******************************************************************
// LOAD: Factory Data
// *******************************************************************

int factory_data_load_fd_flags(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_FLAGS, buf, NVS_SIZE_FD_FLAGS);
    return err;
}

int factory_data_load_fd_tag_sn(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_SN, buf, NVS_SIZE_FD_TAG_SN);
    return err;
}

int factory_data_load_fd_tag_model(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_MODEL, buf, NVS_SIZE_FD_TAG_MODEL);
    return err;
}

int factory_data_load_fd_tag_analog_board_id(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_ANALOG_BOARD_ID, buf, NVS_SIZE_FD_TAG_ANALOG_BOARD_ID);
    return err;
}

int factory_data_load_fd_tag_seed(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_SEED, buf, NVS_SIZE_FD_TAG_SEED);
    return err;
}

int factory_data_load_fd_tag_issuer_public_key(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_ISSUER_PUBLIC_KEY, buf, NVS_SIZE_FD_TAG_ISSUER_PUBLIC_KEY);
    return err;
}

int factory_data_load_fd_tag_issuer_certificate(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_ISSUER_CERTIFICATE, buf, NVS_SIZE_FD_TAG_ISSUER_CERTIFICATE);
    return err;
}

int factory_data_load_fd_tag_device_certificate(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_CERTIFICATE, buf, NVS_SIZE_FD_TAG_DEVICE_CERTIFICATE);
    return err;
}

int factory_data_load_fd_tag_device_private_key(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_PRIVATE_KEY, buf, NVS_SIZE_FD_TAG_DEVICE_PRIVATE_KEY);
    return err;
}

int factory_data_load_fd_tag_iot_ca_certificate(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_IOT_CA_CERTIFICATE, buf, NVS_SIZE_FD_TAG_IOT_CA_CERTIFICATE);
    return err;
}

int factory_data_load_fd_tag_device_huk_kcv(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_HUK_KCV, buf, NVS_SIZE_FD_TAG_DEVICE_HUK_KCV);
    return err;
}

int factory_data_load_fd_tag_profiles_issuer_public_key(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY, buf, NVS_SIZE_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY);
    return err;
}

int factory_data_load_fd_tag_base_user_settings(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_BASE_USER_SETTINGS, buf, NVS_SIZE_FD_TAG_BASE_USER_SETTINGS);
    return err;
}

int factory_data_load_fd_tag_default_lisence(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEFAULT_LISENCE, buf, NVS_SIZE_FD_TAG_DEFAULT_LISENCE);
    return err;
}

int factory_data_load_fd_tag_signature(uint8_t *buf)
{
    int err = settings_load_one(NVS_PATH_FACTORY_DATA_FD_TAG_SIGNATURE, buf, NVS_SIZE_FD_TAG_SIGNATURE);
    return err;
}


// *******************************************************************
// LOAD: UUID
// *******************************************************************

int user_data_load_uuid_service(struct bt_uuid_128 *uuid) 
{
	int err = settings_load_one(NVS_PATH_BT_UUID_TRANSPORT_SERVICE, uuid, sizeof(struct bt_uuid_128));
	if (err == 0)
	{
		SEGGER_RTT_printf(0, " --- NVS WRN --- :  UUID Transport Service is NULL\n");
	}	
	else if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  UUID Transport Service Load Failed!\n");
		return err; 
	}

    return err;
}

int user_data_load_uuid_characteristic_in(struct bt_uuid_128 *uuid)
{
	int err = settings_load_one(NVS_PATH_BT_UUID_TRANSPORT_CHARACTERISTIC_IN, uuid, sizeof(struct bt_uuid_128));
	if (err == 0)
	{
		SEGGER_RTT_printf(0, " --- NVS WRN --- :  UUID Transport Characteristic IN is NULL\n");
	}
	else if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  UUID Transport Characteristic IN Load Failed!\n");
		return err; 
	}

    return err;
}

int user_data_load_uuid_characteristic_out(struct bt_uuid_128 *uuid)
{
	int err = settings_load_one(NVS_PATH_BT_UUID_TRANSPORT_CHARACTERISTIC_OUT, uuid, sizeof(struct bt_uuid_128));
	if (err == 0)
	{
		SEGGER_RTT_printf(0, " --- NVS WRN --- :  UUID Transport Characteristic OUT is NULL\n");
	}
	else if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  UUID Transport Characteristic OUT Load Failed!\n");
		return err; 
	}	

    return err;
}



// *******************************************************************
// LOAD: TIME
// *******************************************************************

int user_data_load_time_usb_rx_timeout(uint16_t *usb_rx_timeout)
{
	int err = settings_load_one(NVS_PATH_TIME_USB_RX_TIMEOUT, usb_rx_timeout, sizeof(uint16_t));
	if (err == 0)
	{
		SEGGER_RTT_printf(0, " --- NVS WRN --- :  USB RX Timeout is NULL\n");
	}
	else if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  USB RX Timeout Load Failed!\n");
		return err; 
	}	

    return err;
}

int user_data_load_time_ble_rx_timeout(uint16_t *ble_rx_timeout)
{
	int err = settings_load_one(NVS_PATH_TIME_USB_RX_TIMEOUT, ble_rx_timeout, sizeof(uint16_t));
	if (err == 0)
	{
		SEGGER_RTT_printf(0, " --- NVS WRN --- :  USB RX Timeout is NULL\n");
	}
	else if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  USB RX Timeout Load Failed!\n");
		return err; 
	}

    return err;
}





// *******************************************************************
// SAVE
// *******************************************************************

// UUID
int save_uuid() 
{
	int err = 0;

	struct bt_uuid_128 uuid_bufs[3] = 
	{
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E0, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E1, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
		BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xD134A7E2, 0x1824, 0x4A94, 0xAB73, 0x0637ABC923DF)),
	};

	err = settings_save_one(NVS_PATH_BT_UUID_TRANSPORT_SERVICE, &uuid_bufs[0], sizeof(struct bt_uuid_128));
	if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  UUID Transport Service Save Failed!\n");
		return err; 
	}

	err = settings_save_one(NVS_PATH_BT_UUID_TRANSPORT_CHARACTERISTIC_IN, &uuid_bufs[1], sizeof(struct bt_uuid_128));
	if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  UUID Transport Characteristic IN Save Failed!\n");
		return err; 
	}	

	err = settings_save_one(NVS_PATH_BT_UUID_TRANSPORT_CHARACTERISTIC_OUT, &uuid_bufs[2], sizeof(struct bt_uuid_128));
	if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  UUID Transport Characteristic OUT Save Failed!\n");
		return err; 
	}
	
	return 0;
}

// Time
int save_time() 
{
	int err = 0;

	uint16_t usb_rx_timeout_buf = 500;
	uint16_t ble_rx_timeout_buf = 5000;


	err = settings_save_one(NVS_PATH_TIME_USB_RX_TIMEOUT, &usb_rx_timeout_buf, sizeof(uint16_t));
	if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  USB RX Timeout Save Failed!\n");
		return err; 
	}	

	err = settings_save_one(NVS_PATH_TIME_BLE_RX_TIMEOUT, &ble_rx_timeout_buf, sizeof(uint16_t));
	if (err < 0)
	{
		SEGGER_RTT_printf(0, " --- NVS ERR --- :  BLE RX Timeout Save Failed!\n");
		return err; 
	}		

    return 0;
}

// Factory Data
int save_FD_FLAGS() 
{
    uint8_t buf[NVS_SIZE_FD_FLAGS] = NVS_FD_FLAGS;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_FLAGS, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_SN() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_SN] = NVS_FD_TAG_SN;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_SN, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_MODEL() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_MODEL] = NVS_FD_TAG_MODEL;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_MODEL, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_ANALOG_BOARD_ID() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_ANALOG_BOARD_ID] = NVS_FD_TAG_ANALOG_BOARD_ID;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_ANALOG_BOARD_ID, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_SEED() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_SEED] = { 0 };
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_SEED, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_ISSUER_PUBLIC_KEY() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_ISSUER_PUBLIC_KEY] = NVS_FD_TAG_ISSUER_PUBLIC_KEY;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_ISSUER_PUBLIC_KEY, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_ISSUER_CERTIFICATE() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_ISSUER_CERTIFICATE] = NVS_FD_TAG_ISSUER_CERTIFICATE;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_ISSUER_CERTIFICATE, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_DEVICE_CERTIFICATE() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_DEVICE_CERTIFICATE] = NVS_FD_TAG_DEVICE_CERTIFICATE;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_CERTIFICATE, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_DEVICE_PRIVATE_KEY() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_DEVICE_PRIVATE_KEY] = NVS_FD_TAG_DEVICE_PRIVATE_KEY;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_PRIVATE_KEY, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_IOT_CA_CERTIFICATE() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_IOT_CA_CERTIFICATE] = NVS_FD_TAG_IOT_CA_CERTIFICATE;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_IOT_CA_CERTIFICATE, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_DEVICE_HUK_KCV() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_DEVICE_HUK_KCV] = { 0 };
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEVICE_HUK_KCV, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY] = NVS_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_BASE_USER_SETTINGS() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_BASE_USER_SETTINGS] = { 0 };
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_BASE_USER_SETTINGS, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_DEFAULT_LISENCE() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_DEFAULT_LISENCE] = { 0 };
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_DEFAULT_LISENCE, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}

int save_FD_TAG_SIGNATURE() 
{
    uint8_t buf[NVS_SIZE_FD_TAG_SIGNATURE] = NVS_FD_TAG_SIGNATURE;
	int err = settings_save_one(NVS_PATH_FACTORY_DATA_FD_TAG_SIGNATURE, buf, sizeof(buf));
	if (err < 0)
	{
		return err; 
	}   
    return 0;
}



int settings_init_save()
{
    int err = 0;

    err = save_FD_FLAGS();
    if (err < 0)
    {
        return err;
    }
    err = save_FD_TAG_SN();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_MODEL();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_ANALOG_BOARD_ID();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_SEED();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_ISSUER_PUBLIC_KEY();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_ISSUER_CERTIFICATE();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_DEVICE_CERTIFICATE();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_DEVICE_PRIVATE_KEY();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_IOT_CA_CERTIFICATE();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_DEVICE_HUK_KCV();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_PROFILES_ISSUER_PUBLIC_KEY();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_BASE_USER_SETTINGS();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_DEFAULT_LISENCE();
    if (err < 0)
    {
        return err;
    }    
    err = save_FD_TAG_SIGNATURE();
    if (err < 0)
    {
        return err;
    }    

	err = save_uuid();
    if (err < 0)
    {
        return err;
    }    
	err = save_time();
    if (err < 0)
    {
        return err;
    }

	return 0;
}