/*
 * ble_part.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "ble_part.h"


static struct bt_conn *conns[CONFIG_BT_MAX_CONN];

static struct k_work restart_adv_work;

static uint8_t initialized = 0;
static uint8_t connected = 0;

// Сервис - TRANSPORT
static ssize_t BT_TRANSPORT_write_state(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr, const void *buf,
                                        uint16_t len, uint16_t offset, uint8_t flags);

static ssize_t BT_TRANSPORT_read_state(struct bt_conn *conn,
                                       const struct bt_gatt_attr *attr, void *buf,
                                       uint16_t len, uint16_t offset);

static void BT_NOTIFICATIONS_TRANSPORT_OUT(const struct bt_gatt_attr *attr, uint16_t value);



// *******************************************************************
// Adverstiment
// *******************************************************************

static struct bt_le_ext_adv *adv;

const struct bt_le_adv_param adv_params = {
    .options = BT_LE_ADV_OPT_CONN,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
};
static uint8_t adv_flags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;

static struct bt_data adv_data[4]; 


// TRANSPORT 
static struct bt_gatt_service transport_service;

static struct bt_gatt_chrc TRANSPORT_CHARACTERITIC_IN;
static struct bt_gatt_chrc TRANSPORT_CHARACTERITIC_OUT;

static struct bt_gatt_ccc_managed_user_data TRANSPORT_CCC = BT_GATT_CCC_MANAGED_USER_DATA_INIT(BT_NOTIFICATIONS_TRANSPORT_OUT, NULL, NULL);;


static uint8_t BT_TRANSPORT_In;
static uint8_t BT_TRANSPORT_Out;

static uint8_t transport_notify_out = 0;

struct bt_gatt_attr TRANSPORT_ATTRIBUTES[] = 
{
    BT_GATT_PRIMARY_SERVICE(NULL),

    BT_GATT_CHARACTERISTIC(NULL,
                        BT_GATT_CHRC_WRITE,
                        BT_GATT_PERM_WRITE,
                        BT_TRANSPORT_read_state, BT_TRANSPORT_write_state, &BT_TRANSPORT_In),

    BT_GATT_CHARACTERISTIC(NULL,
                        BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                        BT_GATT_PERM_READ,
                        BT_TRANSPORT_read_state, BT_TRANSPORT_write_state, &BT_TRANSPORT_Out),                        

    BT_GATT_CCC_MANAGED(&TRANSPORT_CCC, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
};


static tUniversalMessageRX poolBuffersRX_BLE[COUNT_BLE_RX_POOL_BUFFERS];  // Буферы приёма
static tUniversalMessageRX *currentPoolBufferRX_BLE = NULL;               // Текущий Буфер

extern struct k_msgq parser_queue;  // Очередь парсера


// BAS
static uint8_t BT_BAS_BatteryLevel = 0;
static uint8_t BT_BAS_BatteryLevelStatus = 0x01;

static uint8_t bas_notify_battery_level = 0;
static uint8_t bas_notify_battery_level_status = 0;



// *******************************************************************
// Коллбэки
// *******************************************************************

static void BT_MTU_UPDATED(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
    printk(" --- BLE --- :  Updated MTU: TX %d RX %d bytes\n", tx, rx);
}

// Подключение/Отключение
static void BT_CONNECTED(struct bt_conn *conn, uint8_t err)
{
    char addr_str[BT_ADDR_STR_LEN];
    connected = 1;


    if (err == 0)
    {
        for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
        {
            if (!conns[i]) 
            {
                conns[i] = bt_conn_ref(conn);
                bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, BT_ADDR_STR_LEN);

                printk(" --- BLE --- :  Connected: %s\n", addr_str);

                return;
            }
        }     
    }

    printk(" --- BLE ERR --- :  Bluetooth connected Failed!\n");
}

static void BT_DISCONNECTED(struct bt_conn *conn, uint8_t reason)
{
    char addr_str[BT_ADDR_STR_LEN];
    int err= 0;

    connected = 0;

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++)
    {
        if (conns[i] == conn)
        {
            bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, BT_ADDR_STR_LEN);
            bt_conn_unref(conn);
            conns[i] = NULL;

            k_work_submit(&restart_adv_work); // Перезапускаем Advertisement

            printk(" --- BLE --- :  Disconnected: %s\n", addr_str);

            return;
        }
    }

    printk(" --- BLE ERR --- :  Bluetooth device delete Failed!\n");
}



// Сервис - TRANSPORT
static ssize_t BT_TRANSPORT_read_state(struct bt_conn *conn,
                                       const struct bt_gatt_attr *attr, void *buf,
                                       uint16_t len, uint16_t offset)
{
    const uint8_t *val = (uint8_t*)attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, val, sizeof(*val));
}





static ssize_t BT_TRANSPORT_write_state(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr, const void *buf,
                                        uint16_t len, uint16_t offset, uint8_t flags)
{    
    static uint8_t *ptrData;
    static uint8_t byteBuf;

    static uint16_t lenPayloadBuf;
    static uint16_t crcValueBuf;

    static uint8_t buildPacket = 0;

    int err = 0; 


    if (len >= MESSAGE_RX_BUFFER_SIZE)
	{
		printk("Too big message!\n");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

    ptrData = (uint8_t*)buf;
    for (int i = 0; i < len; i++) 
    {
        byteBuf = ptrData[i];

        if (!buildPacket) 
        {
            if (byteBuf == PROTOCOL_PREAMBLE)
            {
                if (getUnusedBuffer(&currentPoolBufferRX_BLE, poolBuffersRX_BLE, COUNT_BLE_RX_POOL_BUFFERS) == 0)
                {
                    buildPacket = 1;
                    currentPoolBufferRX_BLE->length = 0;
                    currentPoolBufferRX_BLE->inUse = 1;
                }
                else 
                    printk(" --- BLE ERR --- : There are no free buffers!\n");
            }
            else
                continue;
        }

        currentPoolBufferRX_BLE->data[ currentPoolBufferRX_BLE->length ] = byteBuf;

        if (currentPoolBufferRX_BLE->length == PROTOCOL_INDEX_PL_LEN_START) // LEN MSB
        {
            lenPayloadBuf = byteBuf;
            lenPayloadBuf <<= 8;
        }
        else if (currentPoolBufferRX_BLE->length == PROTOCOL_INDEX_PL_LEN_START + 1) // LEN LSB
        {
            lenPayloadBuf |= byteBuf;
        }
        else if (currentPoolBufferRX_BLE->length > PROTOCOL_INDEX_PL_LEN_START + 1)
        {
            if (currentPoolBufferRX_BLE->length >= lenPayloadBuf + PROTOCOL_INDEX_HEAD_LENGTH) 
            {
                if (currentPoolBufferRX_BLE->length == lenPayloadBuf+PROTOCOL_INDEX_HEAD_LENGTH) // End Mark
                {
                    if (byteBuf != PROTOCOL_END_MARK) 
                        printk(" --- BLE ERR --- :  Bad END MARK byte for recieved Packet!\n");
                }
                else if (currentPoolBufferRX_BLE->length == lenPayloadBuf+PROTOCOL_INDEX_HEAD_LENGTH+1) // CRC MSB 
                {
                    crcValueBuf = byteBuf;
                    crcValueBuf <<= 8;
                }
                else if (currentPoolBufferRX_BLE->length == lenPayloadBuf+PROTOCOL_INDEX_HEAD_LENGTH+2) // CRC LSB 
                {
                    crcValueBuf |= byteBuf;

                    uint16_t crcCalculatedBuf = calculateCRC(currentPoolBufferRX_BLE->data, lenPayloadBuf+PROTOCOL_INDEX_HEAD_LENGTH+1);

                    if (crcValueBuf != crcCalculatedBuf) 
                        printk(" --- BLE ERR --- :  Bad CRC for recieved Packet!\n");


                    err = k_msgq_put(&parser_queue, &currentPoolBufferRX_BLE, K_NO_WAIT);
                    if (err != 0)
                       printk(" --- PARSER ERR --- :  Parser queue put error: %d\n", err);

                    buildPacket = 0; // Конец
                }                
            }
        }

        currentPoolBufferRX_BLE->length++;
    }

    return len;
}

// Notification - TRANSPORT
static void BT_NOTIFICATIONS_TRANSPORT_OUT(const struct bt_gatt_attr *attr, uint16_t value)
{
    uint8_t notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;

    transport_notify_out = notify;
	printk(" --- BLE Notification --- :  Transport OUT Notification %s\n", notify ? "enabled" : "disabled");
}



// Сервис - BAS
static ssize_t BT_BAS_read_state(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset)
{
    uint8_t *val = (uint8_t*)attr->user_data;
    if (!val) return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, val, sizeof(*val));
}

// Notification - BAS
static void BT_NOTIFICATIONS_BAS_BATTERY_LEVEL(const struct bt_gatt_attr *attr, uint16_t value)
{
    uint8_t notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;

    bas_notify_battery_level = notify;
    printk(" --- BLE Notification --- :  Battery Level Notification %s\n", notify ? "enabled" : "disabled");
}

static void BT_NOTIFICATIONS_BAS_BATTERY_LEVEL_STATUS(const struct bt_gatt_attr *attr, uint16_t value) 
{
    uint8_t notify = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;

    bas_notify_battery_level_status = notify;
    printk(" --- BLE Notification --- :  Battery Level Status Notification %s\n", notify ? "enabled" : "disabled");    
}



// *******************************************************************
// GATT сервисы
// *******************************************************************

BT_GATT_SERVICE_DEFINE(bas_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
    
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           BT_BAS_read_state, NULL, &BT_BAS_BatteryLevel),
    BT_GATT_CCC(BT_NOTIFICATIONS_BAS_BATTERY_LEVEL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           BT_BAS_read_state, NULL, &BT_BAS_BatteryLevelStatus),
    BT_GATT_CCC(BT_NOTIFICATIONS_BAS_BATTERY_LEVEL_STATUS, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);



// *******************************************************************
// Инициализация BLE
// *******************************************************************

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = BT_CONNECTED,
    .disconnected = BT_DISCONNECTED,
};

static struct bt_gatt_cb gatt_callbacks = {
    .att_mtu_updated = BT_MTU_UPDATED,
};


static void restart_adv(struct k_work *work)
{
    uint8_t err;

    bt_le_ext_adv_stop(adv);

    err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err < 0)
        printk(" --- BLE ERR --- :  Failed to restart advertising: %d\n", err);
    else
        printk(" --- BLE --- :  Successful restart advertising\n");
}


int ble_prepare_transport_service(struct bt_uuid_128 *transport_service_uuid, 
            struct bt_uuid_128 *transport_characteristic_in_uuid, 
            struct bt_uuid_128 *transport_characteristic_out_uuid)
{
    struct bt_gatt_chrc *chrc_buffer;


    TRANSPORT_ATTRIBUTES[0].user_data = (void *)transport_service_uuid;

    chrc_buffer = (struct bt_gatt_chrc*)(TRANSPORT_ATTRIBUTES[1].user_data);
    chrc_buffer->uuid = &transport_characteristic_in_uuid->uuid;

    chrc_buffer = (struct bt_gatt_chrc*)(TRANSPORT_ATTRIBUTES[3].user_data);
    chrc_buffer->uuid = &transport_characteristic_out_uuid->uuid;


    transport_service.attrs = TRANSPORT_ATTRIBUTES;
    transport_service.attr_count = ARRAY_SIZE(TRANSPORT_ATTRIBUTES);
    return bt_gatt_service_register(&transport_service);
}



int ble_begin(struct bt_uuid_128 *transport_service_uuid, 
            struct bt_uuid_128 *transport_characteristic_in_uuid, 
            struct bt_uuid_128 *transport_characteristic_out_uuid)
{
    int err= 0;
    
    k_work_init(&restart_adv_work, restart_adv);


    // Настраиваем Advertisement
    adv_data[0].type = BT_DATA_FLAGS;
    adv_data[0].data_len = 1;
    adv_data[0].data = &adv_flags;

    adv_data[1].type = BT_DATA_NAME_COMPLETE;
    adv_data[1].data_len = strlen(CONFIG_BT_DEVICE_NAME);
    adv_data[1].data = (const uint8_t *)CONFIG_BT_DEVICE_NAME;

    adv_data[2].type = BT_DATA_UUID16_ALL;
    adv_data[2].data_len = BT_UUID_SIZE_16;
    adv_data[2].data = (const uint8_t *)BT_UUID_16_ENCODE(BT_UUID_BAS_VAL);

    adv_data[3].type = BT_DATA_UUID128_SOME;
    adv_data[3].data_len = BT_UUID_SIZE_128;
    adv_data[3].data = transport_service_uuid->val;    


    // Инициализируем и запускаем стек
    err = bt_enable(NULL);
    if (err < 0) 
    {
        printk(" --- BLE ERR --- :  Bluetooth enable Failed: %d\n", err);
        return err;
    }

    // Добавляем транспортный сервис
    err = ble_prepare_transport_service(transport_service_uuid, transport_characteristic_in_uuid, transport_characteristic_out_uuid);
    if (err < 0)
    {
        printk(" --- BLE ERR --- :  Register Transport Service Failed: %d\n", err);
        return err;
    }
    else 
        printk(" --- BLE --- :  Transport Service successful added\n");

        
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
    if (err < 0) 
    {
        printk(" --- BLE ERR --- :  Advertisement create Failed: %d\n", err);
        return err;
    }


	err = bt_le_ext_adv_set_data(adv, adv_data, ARRAY_SIZE(adv_data), NULL, 0);
    if (err < 0) 
    {
        printk(" --- BLE ERR --- :  Set data to advertising Failed: %d\n", err);
        return err;
    }

    bt_gatt_cb_register(&gatt_callbacks);


	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err < 0) 
    {
        printk(" --- BLE ERR --- :  Begin advetisement start Failed: %d\n", err);
        return err;
    }

    initialized = 1;
    printk(" --- BLE --- :  Successful Initialized!\n");

    return 0;    
}



// *******************************************************************
// Обёртки для сервисов
// *******************************************************************

uint8_t ble_bas_get_battery_level() 
{
    return BT_BAS_BatteryLevel;
}

uint8_t ble_bas_get_battery_level_status() 
{
    return BT_BAS_BatteryLevelStatus;
}


int ble_bas_set_battery_level(uint8_t value) 
{
    int err= 0;
    
    if (!connected)
        return -EAGAIN;

    BT_BAS_BatteryLevel = value;

    if (bas_notify_battery_level)
       err = bt_gatt_notify(NULL, &bas_service.attrs[2], &BT_BAS_BatteryLevel, sizeof(BT_BAS_BatteryLevel));
    return err;
}

int ble_bas_set_battery_level_status(uint8_t value) 
{
    int err= 0;
    
    if (!connected)
        return -EAGAIN;

    BT_BAS_BatteryLevelStatus = value & 0x0F;

    if (bas_notify_battery_level_status)
        err = bt_gatt_notify(NULL, &bas_service.attrs[4], &BT_BAS_BatteryLevelStatus, sizeof(BT_BAS_BatteryLevelStatus));
    return err;
}