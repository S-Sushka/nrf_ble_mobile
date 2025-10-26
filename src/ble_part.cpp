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

static struct bt_data *ad = nullptr; 
static size_t ad_count = 0;

static struct bt_gatt_service *dynamic_services = nullptr;
static size_t dynamic_services_count = 0;

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

    if (!err)
    {
        ble_device = bt_conn_ref(conn);
        printk(" --- BLE --- :  Connected\n\n");
    } 
    else
        printk(" --- BLE ERR --- :  Bluetooth connected Failed!\n");
}

static void BT_DISCONNECTED(struct bt_conn *conn, uint8_t reason)
{
    int err = 0;
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
    int err;

    bt_le_ext_adv_stop(adv);

    err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err)
        printk(" --- BLE ERR --- :  Failed to restart advertising: %d\n", err);
    else
        printk(" --- BLE --- :  Successful restart advertising\n\n");
}


static uint8_t flags = BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR;
int fill_advertisement_data() 
{
    ad = (bt_data *)k_malloc(3 * sizeof(bt_data));

    if (ad != nullptr) 
    {
        ad[0].type = BT_DATA_FLAGS;
        ad[0].data_len = 1;
        ad[0].data = &flags;

        ad[1].type = BT_DATA_NAME_COMPLETE;
        ad[1].data_len = strlen(CONFIG_BT_DEVICE_NAME);
        ad[1].data = (const uint8_t *)CONFIG_BT_DEVICE_NAME;

        ad[2].type = BT_DATA_UUID16_ALL;
        ad[2].data_len = BT_UUID_SIZE_16;
        ad[2].data = (const uint8_t *)BT_UUID_16_ENCODE(BT_UUID_BAS_VAL);

        ad_count = 3;
        return 0;
    }
    else 
        return ENOSR;
}

int ble_begin(void)
{
    int err = 0;
    
    k_work_init(&restart_adv_work, restart_adv);

    err = fill_advertisement_data();
    if (err) 
    {
        printk(" --- BLE ERR --- :  Fill advertising with data Failed: %d\n", err);
        return err;
    }

    err = bt_enable(NULL);
    if (err) 
    {
        printk(" --- BLE ERR --- :  Bluetooth enable Failed: %d\n", err);
        return err;
    }

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
    if (err) 
    {
        printk(" --- BLE ERR --- :  Advertisement create Failed: %d\n", err);
        return err;
    }
    
	err = bt_le_ext_adv_set_data(adv, ad, ad_count, NULL, 0);
    if (err) 
    {
        printk(" --- BLE ERR --- :  Set data to advertising Failed: %d\n", err);
        return err;
    }

    bt_gatt_cb_register(&gatt_callbacks);


    
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err) 
    {
        printk(" --- BLE ERR --- :  Begin advetisement start Failed: %d\n", err);
        return err;
    }

    initialized = true;
    printk(" --- BLE --- :  Successful Initialized!\n\n");
    return 0;    
}



int ble_add_service(bt_uuid_128 *service_uuid, bt_gatt_attr *attributes, size_t attributes_len)
{
    if (initialized) 
    {
        printk(" --- BLE ERR --- :  You can create dynamic services before bt_le_ext_adv_start() only");
        return EAGAIN;
    }
    else 
    {
        int err = 0;

        // Выделяем место под сервис
        if (dynamic_services_count == 0)
            dynamic_services = (bt_gatt_service *)k_malloc(1 * sizeof(bt_gatt_service));
        else
            dynamic_services = (bt_gatt_service *)k_realloc(dynamic_services, (dynamic_services_count+1) * sizeof(bt_gatt_service));

        if (dynamic_services == nullptr) 
        {
           printk(" --- BLE ERR --- :  Find memory to add dynamic GATT service Failed\n");
            return ENOSR;
        }


        bt_gatt_service test_service = {
            .attrs = attributes,
            .attr_count = attributes_len,
        };

        dynamic_services[dynamic_services_count].attrs = attributes;
        dynamic_services[dynamic_services_count].attr_count = attributes_len;

        err = bt_gatt_service_register(&dynamic_services[dynamic_services_count]);   
        if (err) 
        {
            printk(" --- BLE ERR --- :  Register new dynamic GATT service Failed: %d\n", err);
            return err;
        }
        dynamic_services_count++;

        // Добавляем сервис в Adversiment
        ad = (bt_data *)k_realloc(ad, (ad_count+1) * sizeof(bt_data));
        if (ad == nullptr) 
        {
            printk(" --- BLE ERR --- :  Adding new dynamic GATT service to Advertisement Failed: %d\n", err);
            return ENOSR;
        }
        ad_count++;

        ad[ad_count-1].type = BT_DATA_UUID128_SOME;
        ad[ad_count-1].data_len = BT_UUID_SIZE_128;
        ad[ad_count-1].data = service_uuid->val;

        printk(" --- BLE --- :  New dynamic GATT service successful added\n");
        return 0;
    }
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
    int err = 0;
    
    if (!connected)
        return EAGAIN;

    BT_BAS_BatteryLevel = value;

    if (bas_notify_battery_level)
        err = bt_gatt_notify(ble_device, &bas_service.attrs[2], &BT_BAS_BatteryLevel, sizeof(BT_BAS_BatteryLevel));
    return err;
}

int ble_bas_set_battery_level_status(uint8_t value) 
{
    int err = 0;
    
    if (!connected)
        return EAGAIN;

    BT_BAS_BatteryLevelStatus = value & 0x0F;

    if (bas_notify_battery_level_status)
        err = bt_gatt_notify(ble_device, &bas_service.attrs[4], &BT_BAS_BatteryLevelStatus, sizeof(BT_BAS_BatteryLevelStatus));
    return err;
}