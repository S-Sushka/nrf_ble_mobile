#include "ble_part.h"


struct bt_conn *ble_device = nullptr;

static struct k_work restart_adv_work;

static bool initialized = false;
static bool connected = false;



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
static bt_gatt_service transport_service;

static struct bt_gatt_chrc TRANSPORT_CHARACTERITIC_IN;
static struct bt_gatt_chrc TRANSPORT_CHARACTERITIC_OUT;
static struct _bt_gatt_ccc TRANSPORT_CCC;

static struct bt_gatt_attr TRANSPORT_ATTRIBUTES[6];


static uint8_t BT_TRANSPORT_In;
static uint8_t BT_TRANSPORT_Out;

static bool transport_notify_out = false;


// BAS
static uint8_t BT_BAS_BatteryLevel = 0;
static uint8_t BT_BAS_BatteryLevelStatus = 0x01;

static bool bas_notify_battery_level = false;
static bool bas_notify_battery_level_status = false;



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
    connected = true;

    if (err == 0)
    {
        ble_device = bt_conn_ref(conn);
        printk(" --- BLE --- :  Connected\n");
    } 
    else
        printk(" --- BLE ERR --- :  Bluetooth connected Failed!\n");
}

static void BT_DISCONNECTED(struct bt_conn *conn, uint8_t reason)
{
    int err= 0;
    connected = false;

    if (ble_device) 
    {
        bt_conn_unref(conn);
        ble_device = nullptr;

        printk(" --- BLE --- :  Disconnected\n");
    }
    else 
        printk(" --- BLE ERR --- :  Bluetooth device delete Failed!\n");


    k_work_submit(&restart_adv_work);
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
    if (offset != 0 || len != 1)
	{
		printk("Too big message! Maximum is 1 byte\n");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

    BT_TRANSPORT_In = *((uint8_t*)buf);
	printk("Transport IN New Value: %d\n", BT_TRANSPORT_In);

	BT_TRANSPORT_Out = BT_TRANSPORT_In;
	if (transport_notify_out)
		bt_gatt_notify(ble_device, &TRANSPORT_ATTRIBUTES[4], &BT_TRANSPORT_Out, sizeof(BT_TRANSPORT_Out));

    return len;
}

// Notification - TRANSPORT
static void BT_NOTIFICATIONS_TRANSPORT_OUT(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notify = (value == BT_GATT_CCC_NOTIFY) ? true : false;

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
    bool notify = (value == BT_GATT_CCC_NOTIFY) ? true : false;

    bas_notify_battery_level = notify;
    printk(" --- BLE Notification --- :  Battery Level Notification %s\n", notify ? "enabled" : "disabled");
}

static void BT_NOTIFICATIONS_BAS_BATTERY_LEVEL_STATUS(const struct bt_gatt_attr *attr, uint16_t value) 
{
    bool notify = (value == BT_GATT_CCC_NOTIFY) ? true : false;

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


int ble_prepare_transport_service(bt_uuid_128 *transport_service_uuid, 
            bt_uuid_128 *transport_characteristic_in_uuid, 
            bt_uuid_128 *transport_characteristic_out_uuid)
{
	TRANSPORT_CHARACTERITIC_IN =
		{
			.uuid = &transport_characteristic_in_uuid->uuid,
			.value_handle = 0, // Заполнится стеком
			.properties = BT_GATT_CHRC_WRITE,
		};

	TRANSPORT_CHARACTERITIC_OUT =
		{
			.uuid = &transport_characteristic_out_uuid->uuid,
			.value_handle = 0, // Заполнится стеком 
			.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		};

	TRANSPORT_CCC = BT_GATT_CCC_INITIALIZER(BT_NOTIFICATIONS_TRANSPORT_OUT, NULL, NULL);

	struct bt_gatt_attr temp[6] =
	{
			BT_GATT_PRIMARY_SERVICE(transport_service_uuid),

			// Характеристика IN
			BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
							  BT_GATT_PERM_READ,
							  bt_gatt_attr_read_chrc,
							  NULL,
							  &TRANSPORT_CHARACTERITIC_IN),

			BT_GATT_ATTRIBUTE(TRANSPORT_CHARACTERITIC_IN.uuid,
							  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
							  BT_TRANSPORT_read_state, BT_TRANSPORT_write_state, &BT_TRANSPORT_In),

			// Характеристика OUT
			BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
							  BT_GATT_PERM_READ,
							  bt_gatt_attr_read_chrc,
							  NULL,
							  &TRANSPORT_CHARACTERITIC_OUT),

			BT_GATT_ATTRIBUTE(TRANSPORT_CHARACTERITIC_OUT.uuid,
							  BT_GATT_PERM_READ,
							  BT_TRANSPORT_read_state, BT_TRANSPORT_write_state, &BT_TRANSPORT_Out),

			// Настройка Notification
			BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC,
							  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
							  bt_gatt_attr_read_ccc,
							  bt_gatt_attr_write_ccc,
							  &TRANSPORT_CCC),
	};
	memcpy(TRANSPORT_ATTRIBUTES, temp, sizeof(temp));

    transport_service.attrs = TRANSPORT_ATTRIBUTES;
    transport_service.attr_count = ARRAY_SIZE(TRANSPORT_ATTRIBUTES);

    return bt_gatt_service_register(&transport_service);
}


int ble_begin(bt_uuid_128 *transport_service_uuid, 
            bt_uuid_128 *transport_characteristic_in_uuid, 
            bt_uuid_128 *transport_characteristic_out_uuid)
{
    int err= 0;
    
    k_work_init(&restart_adv_work, restart_adv);


    // Добавляем транспортный сервис
    err = ble_prepare_transport_service(transport_service_uuid, transport_characteristic_in_uuid, transport_characteristic_out_uuid);
    if (err < 0)
    {
        printk(" --- BLE ERR --- :  Register Transport Service Failed: %d\n", err);
        return err;
    }
    else 
        printk(" --- BLE --- :  Transport Service successful added\n");


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

    initialized = true;
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
        err = bt_gatt_notify(ble_device, &bas_service.attrs[2], &BT_BAS_BatteryLevel, sizeof(BT_BAS_BatteryLevel));
    return err;
}

int ble_bas_set_battery_level_status(uint8_t value) 
{
    int err= 0;
    
    if (!connected)
        return -EAGAIN;

    BT_BAS_BatteryLevelStatus = value & 0x0F;

    if (bas_notify_battery_level_status)
        err = bt_gatt_notify(ble_device, &bas_service.attrs[4], &BT_BAS_BatteryLevelStatus, sizeof(BT_BAS_BatteryLevelStatus));
    return err;
}