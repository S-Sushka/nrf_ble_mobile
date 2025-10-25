#include "ble_part.h"


struct bt_conn *ble_device = nullptr;

const struct bt_data sd[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
            strlen(CONFIG_BT_DEVICE_NAME)),
};

const struct bt_le_adv_param adv_params = {
    .options = BT_LE_ADV_OPT_CONN,
    .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
    .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
};

static struct bt_data *ptr_new_ad;


// *******************************************************************
// Коллбэки
// *******************************************************************

// BAS
static uint8_t BT_BAS_BatteryLevel = 12;
static uint8_t BT_BAS_BatteryLevelStatus = 34;

static ssize_t BT_BAS_read_state(struct bt_conn *conn,
                                 const struct bt_gatt_attr *attr, void *buf,
                                 uint16_t len, uint16_t offset)
{
    const uint8_t *val = (uint8_t*)attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, val, sizeof(*val));
}

static ssize_t BT_BAS_write_state(struct bt_conn *conn,
                                  const struct bt_gatt_attr *attr, const void *buf,
                                  uint16_t len, uint16_t offset, uint8_t flags)
{
    printk("Error BAS - You can’t write to BAS!\n");
    return BT_GATT_ERR(0);
}


// MAIN TASK
static uint8_t BT_MAIN_TASK_In;
static uint8_t BT_MAIN_TASK_Out;

static ssize_t BT_MAIN_TASK_read_state(struct bt_conn *conn,
                                       const struct bt_gatt_attr *attr, void *buf,
                                       uint16_t len, uint16_t offset)
{
    const uint8_t *val = (uint8_t*)attr->user_data;
    return bt_gatt_attr_read(conn, attr, buf, len, offset, val, sizeof(*val));
}

static ssize_t BT_MAIN_TASK_write_state(struct bt_conn *conn,
                                        const struct bt_gatt_attr *attr, const void *buf,
                                        uint16_t len, uint16_t offset, uint8_t flags)
{
    if (offset != 0 || len != 1) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    BT_MAIN_TASK_In = *((uint8_t*)buf);

    return len;
}


static void BT_NOTIFICATIONS(const struct bt_gatt_attr *attr, uint16_t value)
{
    printk("Notification %s\n", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}


static void BT_CONNECTED(struct bt_conn *conn, uint8_t err)
{
    if (!err)
        ble_device = bt_conn_ref(conn);        
}

static void BT_DISCONNECTED(struct bt_conn *conn, uint8_t reason)
{
    bt_conn_unref(conn);
    ble_device = nullptr;

    bt_le_adv_start(&adv_params, ptr_new_ad, ARRAY_SIZE(ptr_new_ad), sd, ARRAY_SIZE(sd));
}


static void BT_MTU_UPDATED(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
    printk("Updated MTU: TX %d RX %d bytes\n", tx, rx);
}


BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = BT_CONNECTED,
    .disconnected = BT_DISCONNECTED,
};

static struct bt_gatt_cb gatt_callbacks = {
    .att_mtu_updated = BT_MTU_UPDATED,
};


// *******************************************************************
// GATT сервисы
// *******************************************************************

BT_GATT_SERVICE_DEFINE(bas_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_BAS),
    
    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           BT_BAS_read_state, BT_BAS_write_state, &BT_BAS_BatteryLevel),
    BT_GATT_CCC(BT_NOTIFICATIONS, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    BT_GATT_CHARACTERISTIC(BT_UUID_BAS_BATTERY_LEVEL_STATUS,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           BT_BAS_read_state, BT_BAS_write_state, &BT_BAS_BatteryLevelStatus),
    BT_GATT_CCC(BT_NOTIFICATIONS, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE)
);

// *******************************************************************
// Adverstiments
// *******************************************************************

const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};




// *******************************************************************
// Инициализация BLE
// *******************************************************************

int ble_begin(void)
{
    int err = 0;
    
    err = bt_enable(NULL);
    if (err) return err;

   bt_gatt_cb_register(&gatt_callbacks);

	err = bt_le_adv_start(&adv_params, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) return err; 

    return 0;    
}


static struct bt_gatt_service BT_MAIN_TASK_DYNAMIC_SERVICE;
int ble_create_main_task_service(bt_uuid_128 *uuid_service, bt_uuid_128 *uuid_characteristic_in, bt_uuid_128 *uuid_characteristic_out) 
{
    // Инициализируем структуры для Характеристик/CCC
	static struct bt_gatt_chrc MAIN_TASK_CHARACTERITIC_IN = 
	{
		.uuid = &uuid_characteristic_in->uuid,
		.value_handle = 0, /* Заполнится стеком */
		.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
	};

	static struct bt_gatt_chrc MAIN_TASK_CHARACTERITIC_OUT = 
	{
		.uuid = &uuid_characteristic_out->uuid,
		.value_handle = 0, /* Заполнится стеком */
		.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
	};	

	static struct _bt_gatt_ccc MAIN_TASK_CCC = BT_GATT_CCC_INITIALIZER(BT_NOTIFICATIONS, NULL, NULL);


    // Инициализируем массив аттрибутов
	static struct bt_gatt_attr MAIN_TASK_ATTRIBUTES[] = 
	{
		BT_GATT_PRIMARY_SERVICE(uuid_service),

        // Характеристика IN
        BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
                        BT_GATT_PERM_READ,
                        bt_gatt_attr_read_chrc,
                        NULL,
                        &MAIN_TASK_CHARACTERITIC_IN),

        BT_GATT_ATTRIBUTE(&uuid_characteristic_in->uuid,
                        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                        BT_MAIN_TASK_read_state, BT_MAIN_TASK_write_state, &BT_MAIN_TASK_In),

        // Характеристика OUT
        BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
                        BT_GATT_PERM_READ,
                        bt_gatt_attr_read_chrc,
                        NULL,
                        &MAIN_TASK_CHARACTERITIC_OUT),

        BT_GATT_ATTRIBUTE(&uuid_characteristic_out->uuid,
                        BT_GATT_PERM_READ,
                        BT_MAIN_TASK_read_state, BT_MAIN_TASK_write_state, &BT_MAIN_TASK_Out),


        // Настройка Notification
        BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC,
                        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
                        bt_gatt_attr_read_ccc,
                        bt_gatt_attr_write_ccc,
                        &MAIN_TASK_CCC),
	};

    // Регистрируем Сервис
 	BT_MAIN_TASK_DYNAMIC_SERVICE = BT_GATT_SERVICE(MAIN_TASK_ATTRIBUTES);
	bt_gatt_service_register(&BT_MAIN_TASK_DYNAMIC_SERVICE);   


    // Инициируем новую Adversiment
	bt_le_adv_stop();
    static struct bt_data new_ad[3] = 
    {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
        BT_DATA(BT_DATA_UUID128_ALL,
				uuid_service->val,
				16)
    };
    ptr_new_ad = new_ad;

	int err = bt_le_adv_start(&adv_params, new_ad, sizeof(new_ad), sd, ARRAY_SIZE(sd));    
    return err;
}





// *******************************************************************
// DEBUG
// *******************************************************************

void ble_test_notify(uint8_t value) 
{
	int err = 0;

    if (ble_device) 
    {
        BT_MAIN_TASK_Out = value;
        err = bt_gatt_notify(ble_device, &BT_MAIN_TASK_DYNAMIC_SERVICE.attrs[4], &BT_MAIN_TASK_Out, sizeof(BT_MAIN_TASK_Out));
        if (err != 0)
            printk("Notify Error: %i\n", err);
    }
}

uint16_t prev_mtu = -1;
void ble_test_mtu() 
{
    if (ble_device) 
    {
        if (prev_mtu != bt_gatt_get_uatt_mtu(ble_device))
        {
            printk("MTU: %d\n", bt_gatt_get_uatt_mtu(ble_device));	
            prev_mtu = bt_gatt_get_uatt_mtu(ble_device);
        }    
    }
}