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


static tUniversalMessageRX poolBuffersRX_BLE[COUNT_BLE_RX_POOL_BUFFERS];                // Буферы приёма
static tUniversalMessageRX *currentPoolBuffersRX_BLE[CONFIG_BT_MAX_CONN] = { NULL };    // Текущий буфер для каждого соединения 

static tParcingProcessData context_BLE[CONFIG_BT_MAX_CONN]; // Контекст для выполнения парсинга

extern struct k_msgq parser_queue;  // Очередь парсера



int BT_GET_INDEX_BY_CONN(struct bt_conn *conn) 
{
    bt_conn_ref(conn);
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++)
    {
        if (conns[i] == conn)
        {
            bt_conn_unref(conn);
            return i;
        }
    }
    bt_conn_unref(conn);
    
    return -1;
}



// *******************************************************************
// Сервис - BAS
// *******************************************************************

static ssize_t BT_BAS_read_state(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset);

ssize_t BT_NOTIFICATIONS_WRITE_CB_BATTERY_LEVEL(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags);

ssize_t BT_NOTIFICATIONS_WRITE_CB_BATTERY_LEVEL_STATUS(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags);


static uint8_t BT_BAS_BatteryLevel = 0;
static uint8_t BT_BAS_BatteryLevelStatus = 0x01;

static uint8_t BT_BAS_BATTERY_LEVEL_NOTIFY_States[CONFIG_BT_MAX_CONN] = { 0 };
static struct bt_gatt_ccc_managed_user_data BT_BAS_BATTERY_LEVEL_CCC = BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, NULL, NULL);

static uint8_t BT_BAS_BATTERY_LEVEL_STATUS_NOTIFY_States[CONFIG_BT_MAX_CONN] = { 0 };
static struct bt_gatt_ccc_managed_user_data BT_BAS_BATTERY_LEVEL_STATUS_CCC = BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, NULL, NULL);

BT_GATT_SERVICE_DEFINE(bas_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
    
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           BT_BAS_read_state, NULL, &BT_BAS_BatteryLevel),
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
	 		            bt_gatt_attr_read_ccc, BT_NOTIFICATIONS_WRITE_CB_BATTERY_LEVEL, &BT_BAS_BATTERY_LEVEL_CCC),

    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           BT_BAS_read_state, NULL, &BT_BAS_BatteryLevelStatus),
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
	 		            bt_gatt_attr_read_ccc, BT_NOTIFICATIONS_WRITE_CB_BATTERY_LEVEL_STATUS, &BT_BAS_BATTERY_LEVEL_STATUS_CCC)
);



// *******************************************************************
// Сервис - TRANSPORT
// *******************************************************************

static ssize_t BT_TRANSPORT_write_state(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr, const void *buf,
                                        uint16_t len, uint16_t offset, uint8_t flags);

ssize_t BT_NOTIFICATIONS_WRITE_CB_TRANSPORT(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags);


static struct bt_gatt_service transport_service;

static struct bt_gatt_ccc_managed_user_data TRANSPORT_CCC = BT_GATT_CCC_MANAGED_USER_DATA_INIT(NULL, NULL, NULL);
static uint8_t BT_TRANSPORT_NOTIFY_States[CONFIG_BT_MAX_CONN] = { 0 };

static uint8_t BT_TRANSPORT_defaultWriteBuffer[CONFIG_BT_L2CAP_TX_MTU]; // Без буфера для записи характеристики при обычной записи стек рухнет
struct bt_gatt_attr TRANSPORT_ATTRIBUTES[] = 
{
    BT_GATT_PRIMARY_SERVICE(NULL),

    BT_GATT_CHARACTERISTIC(NULL,
                        BT_GATT_CHRC_WRITE,
                        BT_GATT_PERM_WRITE | BT_GATT_PERM_PREPARE_WRITE,
                        NULL, BT_TRANSPORT_write_state, NULL),

    BT_GATT_CHARACTERISTIC(NULL,
                        BT_GATT_CHRC_NOTIFY,
                        BT_GATT_PERM_READ,
                        NULL, NULL, NULL),
	BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE, \
	 		            bt_gatt_attr_read_ccc, BT_NOTIFICATIONS_WRITE_CB_TRANSPORT, &TRANSPORT_CCC)
};



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


// *******************************************************************
// Коллбэки Стека
// *******************************************************************

static void BT_MTU_UPDATED(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
    SEGGER_RTT_printf(0, " --- BLE --- :  Updated MTU: TX %d RX %d bytes\n", tx, rx);
}

// Подключение/Отключение
static void BT_CONNECTED(struct bt_conn *conn, uint8_t err)
{
    char addr_str[BT_ADDR_STR_LEN];


    if (err == 0)
    {
        for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
        {
            if (!conns[i]) 
            {
                conns[i] = bt_conn_ref(conn);
                bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, BT_ADDR_STR_LEN);

                k_work_submit(&restart_adv_work); // Перезапускаем Advertisement

                SEGGER_RTT_printf(0, " --- BLE --- :  Connected: %s\n", addr_str);

                return;
            }
        }     
    }

    SEGGER_RTT_printf(0, " --- BLE ERR --- :  Bluetooth connected Failed!\n");
}

static void BT_DISCONNECTED(struct bt_conn *conn, uint8_t reason)
{
    char addr_str[BT_ADDR_STR_LEN];


    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++)
    {
        if (conns[i] == conn)
        {
            bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, BT_ADDR_STR_LEN);
            bt_conn_unref(conn);

            conns[i] = NULL;
            if (currentPoolBuffersRX_BLE[i]) 
            {
                currentPoolBuffersRX_BLE[i]->inUse = 0;
                currentPoolBuffersRX_BLE[i] = NULL;
                heapFreeWithCheck(&UniversalHeapRX, currentPoolBuffersRX_BLE[i]->data);
            }            

            BT_TRANSPORT_NOTIFY_States[i] = 0;
            BT_BAS_BATTERY_LEVEL_NOTIFY_States[i] = 0;
            BT_BAS_BATTERY_LEVEL_STATUS_NOTIFY_States[i] = 0;

            k_work_submit(&restart_adv_work); // Перезапускаем Advertisement

            SEGGER_RTT_printf(0, " --- BLE --- :  Disconnected: %s\n", addr_str);

            return;
        }
    }

    SEGGER_RTT_printf(0, " --- BLE ERR --- :  Bluetooth device delete Failed!\n");
}



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

    //bt_le_ext_adv_stop(adv);

    err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err < 0)
        SEGGER_RTT_printf(0, " --- BLE ERR --- :  Failed to restart advertising: %d\n", err);
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
        SEGGER_RTT_printf(0, " --- BLE ERR --- :  Bluetooth enable Failed: %d\n", err);
        return err;
    }

    // Добавляем транспортный сервис
    err = ble_prepare_transport_service(transport_service_uuid, transport_characteristic_in_uuid, transport_characteristic_out_uuid);
    if (err < 0)
    {
        SEGGER_RTT_printf(0, " --- BLE ERR --- :  Register Transport Service Failed: %d\n", err);
        return err;
    }
    SEGGER_RTT_printf(0, " --- BLE --- :  Transport Service successful added\n");

        
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
    if (err < 0) 
    {
        SEGGER_RTT_printf(0, " --- BLE ERR --- :  Advertisement create Failed: %d\n", err);
        return err;
    }


	err = bt_le_ext_adv_set_data(adv, adv_data, ARRAY_SIZE(adv_data), NULL, 0);
    if (err < 0) 
    {
        SEGGER_RTT_printf(0, " --- BLE ERR --- :  Set data to advertising Failed: %d\n", err);
        return err;
    }

    bt_gatt_cb_register(&gatt_callbacks);


	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err < 0) 
    {
        SEGGER_RTT_printf(0, " --- BLE ERR --- :  Begin advetisement start Failed: %d\n", err);
        return err;
    }

    SEGGER_RTT_printf(0, " --- BLE --- :  Successful Initialized!\n");

    return 0;    
}



// *******************************************************************
// Поток Интерфейса
// *******************************************************************

K_MSGQ_DEFINE(ble_queue_tx, sizeof(tUniversalMessageTX), 4, 1);

void ble_thread(void)
{	
	tUniversalMessageTX pkt;
    int conn_index = 0;

    while (1) 
    {
		k_msgq_get(&ble_queue_tx, &pkt, K_FOREVER);
        conn_index = pkt.source - MESSAGE_SOURCE_BLE_CONNS;

        // Notify
        if ( !(conn_index >= 0 && conn_index < CONFIG_BT_MAX_CONN) ) 
        {
            SEGGER_RTT_printf(0, " --- BLE TX ERR --- : Incorrect Source!\n");
            continue;
        }

        if (!conns[conn_index])
            continue;

        if (!pkt.data || pkt.length >= PROTOCOL_MAX_PACKET_LENGTH)
        {
            SEGGER_RTT_printf(0, " --- BLE TX ERR --- : Bad Data!\n");
            continue;
        }        

        bt_conn_ref(conns[conn_index]);
        if (bt_gatt_is_subscribed(conns[conn_index], &transport_service.attrs[4], BT_GATT_CCC_NOTIFY)) 
        {
            int err = bt_gatt_notify(conns[conn_index], &transport_service.attrs[4], pkt.data, pkt.length);
            if (err < 0)
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Sending Notify for \"Transport Out\" Failed: %d\n", err);
        }
        bt_conn_unref(conns[conn_index]);
    }   	
}

K_THREAD_DEFINE(ble_thread_id, THREAD_STACK_SIZE_BLE, ble_thread, NULL, NULL, NULL,
		THREAD_PRIORITY_BLE, 0, 0);



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


void ble_bas_set_battery_level(uint8_t value) 
{
    int err = 0;
    BT_BAS_BatteryLevel = value;

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
    {
        if (!conns[i])
            continue;

        bt_conn_ref(conns[i]);
        if (bt_gatt_is_subscribed(conns[i], &bas_service.attrs[2], BT_GATT_CCC_NOTIFY)) 
        {
            err = bt_gatt_notify(conns[i], &bas_service.attrs[2], &BT_BAS_BatteryLevel, sizeof(BT_BAS_BatteryLevel));
            if (err < 0)
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Sending Notify for \"Battery Level\" Failed: %d\n", err);
        }
        bt_conn_unref(conns[i]);
    }
}

void ble_bas_set_battery_level_status(uint8_t value) 
{
    int err = 0;
    BT_BAS_BatteryLevelStatus = value & 0x0F;

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
    {
        if (!conns[i])
            continue;

        bt_conn_ref(conns[i]);
        if (bt_gatt_is_subscribed(conns[i], &bas_service.attrs[4], BT_GATT_CCC_NOTIFY)) 
        {
            err = bt_gatt_notify(conns[i], &bas_service.attrs[4], &BT_BAS_BatteryLevelStatus, sizeof(BT_BAS_BatteryLevelStatus));
            if (err < 0)
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Sending Notify for \"Battery Level Status\" Failed: %d\n", err);
        }
        bt_conn_unref(conns[i]);
    }
}



// *******************************************************************
// Коллбэки Сервисов
// *******************************************************************

// BAS
static ssize_t BT_BAS_read_state(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset)
{
    uint8_t *val = (uint8_t*)attr->user_data;
    if (!val) return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    return bt_gatt_attr_read(conn, attr, buf, len, offset, val, sizeof(*val));
}

ssize_t BT_NOTIFICATIONS_WRITE_CB_BATTERY_LEVEL(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
    uint16_t value = 0;

    bt_addr_le_t *conn_addr;
    char conn_addr_buf[BT_ADDR_STR_LEN];


    if (len != 2) {  // CCC всегда 2 байта
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    value = ((uint8_t *)buf)[1];
    value <<= 8;
    value |= ((uint8_t *)buf)[0];

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
    {
        if (conns[i] && conns[i] == conn) 
        {
            if (BT_BAS_BATTERY_LEVEL_NOTIFY_States[i] != value)
            {
                conn_addr = (bt_addr_le_t*)bt_conn_get_dst(conns[i]);
                bt_addr_le_to_str(conn_addr, conn_addr_buf, sizeof(conn_addr_buf));

                SEGGER_RTT_printf(0, " --- BLE Notify - BAS Battery Level --- : %s - %s\n", conn_addr_buf, (value == 0) ? "FALSE" : "TRUE");

                BT_BAS_BATTERY_LEVEL_NOTIFY_States[i] = value;
                return bt_gatt_attr_write_ccc(conn, attr, buf, len, offset, flags);
            }
        }
    }

    return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
}


ssize_t BT_NOTIFICATIONS_WRITE_CB_BATTERY_LEVEL_STATUS(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags) 
{
    uint16_t value = 0;

    bt_addr_le_t *conn_addr;
    char conn_addr_buf[BT_ADDR_STR_LEN];


    if (len != 2) {  // CCC всегда 2 байта
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    value = ((uint8_t *)buf)[1];
    value <<= 8;
    value |= ((uint8_t *)buf)[0];

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
    {
        if (conns[i] && conns[i] == conn) 
        {
            if (BT_BAS_BATTERY_LEVEL_STATUS_NOTIFY_States[i] != value)
            {
                conn_addr = (bt_addr_le_t*)bt_conn_get_dst(conns[i]);
                bt_addr_le_to_str(conn_addr, conn_addr_buf, sizeof(conn_addr_buf));

                SEGGER_RTT_printf(0, " --- BLE Notify - BAS Battery Level Status --- : %s - %s\n", conn_addr_buf, (value == 0) ? "FALSE" : "TRUE");

                BT_BAS_BATTERY_LEVEL_STATUS_NOTIFY_States[i] = value;
                return bt_gatt_attr_write_ccc(conn, attr, buf, len, offset, flags);
            }
        }
    }

    return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
}


// Сервис - TRANSPORT
static ssize_t BT_TRANSPORT_write_state(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr, const void *buf,
                                        uint16_t len, uint16_t offset, uint8_t flags)
{
    int err = 0;

    if (flags & BT_GATT_WRITE_FLAG_EXECUTE) 
    {
        return len;
    } 

    int conn_index = BT_GET_INDEX_BY_CONN(conn);    
    if (conn_index < 0) 
    {
        SEGGER_RTT_printf(0, " --- BLE RX ERR --- : Read Data Failed - Incorrect Conn!\n");
        return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
    }

    if (len > PROTOCOL_MAX_PACKET_LENGTH)
	{
		SEGGER_RTT_printf(0, " --- BLE RX ERR --- : Bad Data!\n");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
       

    uint8_t byteBuf = 0;
    for (uint16_t i = 0; i < len; i++)
    {
        byteBuf = ((uint8_t *)buf)[i];

        if (!context_BLE[conn_index].buildPacket)
        {
            if (byteBuf == PROTOCOL_PREAMBLE)
            {
                if (getUnusedBuffer(&currentPoolBuffersRX_BLE[conn_index], poolBuffersRX_BLE, COUNT_BLE_RX_POOL_BUFFERS) == 0)
                {
                    currentPoolBuffersRX_BLE[conn_index]->source = MESSAGE_SOURCE_BLE_CONNS + conn_index;
                    currentPoolBuffersRX_BLE[conn_index]->length = 0;
                    currentPoolBuffersRX_BLE[conn_index]->inUse = 1;

                    context_BLE[conn_index].messageBuf = currentPoolBuffersRX_BLE[conn_index];
                    context_BLE[conn_index].buildPacket = 1;

                    // Сначала выделим и запарсим 5 байт, чтобы узнать длину пакета
                    context_BLE[conn_index].messageBuf->data = k_heap_alloc(&UniversalHeapRX, PROTOCOL_INDEX_PL_START, K_NO_WAIT);
                    if (!context_BLE[conn_index].messageBuf->data)
                    {
                        SEGGER_RTT_printf(0, " --- BLE ERR --- : Allocate memory for buffer Failed!\n");

                        heapFreeWithCheck(&UniversalHeapRX, context_BLE[conn_index].messageBuf->data);
                        return len;
                    }
                }
                else
                {
                    SEGGER_RTT_printf(0, " --- BLE ERR --- : There are no free buffers!\n");

                    heapFreeWithCheck(&UniversalHeapRX, context_BLE[conn_index].messageBuf->data);
                    return len;
                }
            }
            else
            {
                continue;
            }
        }

        // Перераспределяем память под весь пакет
        if (context_BLE[conn_index].messageBuf->length == PROTOCOL_INDEX_PL_START)
        {
            context_BLE[conn_index].messageBuf->data =
                k_heap_realloc(&UniversalHeapRX, context_BLE[conn_index].messageBuf->data, PROTOCOL_INDEX_PL_START + context_BLE[conn_index].lenPayloadBuf + PROTOCOL_END_PART_SIZE, K_NO_WAIT);
            if (!context_BLE[conn_index].messageBuf->data)
            {
                SEGGER_RTT_printf(0, " --- BLE ERR --- : Reallocate memory for buffer Failed!\n");

                heapFreeWithCheck(&UniversalHeapRX, context_BLE[conn_index].messageBuf->data);
                return len;
            }
        }

        err = parseNextByte(byteBuf, &context_BLE[conn_index]);
        if (err < 0)
        {
            switch (err)
            {
            case -EMSGSIZE:
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Recieve Packet too Long!\n");
                break;
            case -EPROTO:
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Bad END MARK byte for recieved Packet!\n");
                break;
            case -EBADMSG:
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Bad CRC for recieved Packet!\n");
                break;
            default:
                SEGGER_RTT_printf(0, " --- BLE ERR --- :  Parse Packet Error: %d\n", err);
                break;
            }

            heapFreeWithCheck(&UniversalHeapRX, context_BLE[conn_index].messageBuf->data);
            return len;
        }
        else if (err > 0)
        {
            err = k_msgq_put(&parser_queue, &context_BLE[conn_index].messageBuf, K_NO_WAIT);
            if (err != 0)
            {
                SEGGER_RTT_printf(0, " --- PARSER ERR --- :  Parser queue put error: %d\n", err);

                heapFreeWithCheck(&UniversalHeapRX, context_BLE[conn_index].messageBuf->data);
                return len;
            }
        }
    }



    if (flags & BT_GATT_WRITE_FLAG_PREPARE) 
    {
        return 0;
    }

    return len;
}

ssize_t BT_NOTIFICATIONS_WRITE_CB_TRANSPORT(struct bt_conn *conn,
			       const struct bt_gatt_attr *attr, const void *buf,
			       uint16_t len, uint16_t offset, uint8_t flags)
{
    uint16_t value = 0;

    bt_addr_le_t *conn_addr;
    char conn_addr_buf[BT_ADDR_STR_LEN];


    if (len != 2) {  // CCC всегда 2 байта
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    value = ((uint8_t *)buf)[1];
    value <<= 8;
    value |= ((uint8_t *)buf)[0];

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) 
    {
        if (conns[i] && conns[i] == conn) 
        {
            if (BT_TRANSPORT_NOTIFY_States[i] != value)
            {
                conn_addr = (bt_addr_le_t*)bt_conn_get_dst(conns[i]);
                bt_addr_le_to_str(conn_addr, conn_addr_buf, sizeof(conn_addr_buf));

                SEGGER_RTT_printf(0, " --- BLE Notify - Transport OUT --- : %s - %s\n", conn_addr_buf, (value == 0) ? "FALSE" : "TRUE");

                BT_TRANSPORT_NOTIFY_States[i] = value;
                return bt_gatt_attr_write_ccc(conn, attr, buf, len, offset, flags);
            }
        }
    }

    return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
}