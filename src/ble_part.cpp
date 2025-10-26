#include "ble_part.h"


struct bt_conn *ble_device = nullptr;

static struct k_work restart_adv_work;

static bool initialized = false;
static bool connected = false;



// *******************************************************************
// Adverstiments
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

// *******************************************************************
// Коллбэки
// *******************************************************************

static void BT_MTU_UPDATED(struct bt_conn *conn, uint16_t tx, uint16_t rx)
{
    printk("Updated MTU: TX %d RX %d bytes\n", tx, rx);
}

// Подключение/Отключение
static void BT_CONNECTED(struct bt_conn *conn, uint8_t err)
{
    connected = true;

    if (!err)
        ble_device = bt_conn_ref(conn);        
}

static void BT_DISCONNECTED(struct bt_conn *conn, uint8_t reason)
{
    int err = 0;
    connected = false;

    if (ble_device) 
    {
        bt_conn_unref(conn);
        ble_device = nullptr;
    }
    else 
        printk("BT delete device Failed\n"); 


    k_work_submit(&restart_adv_work);
}

// Сервис - BAS
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

static void BT_NOTIFICATIONS(const struct bt_gatt_attr *attr, uint16_t value)
{
    printk("Notification %s\n", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}



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
        printk("Failed to restart advertising: %d\n", err);
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
        printk("BT set data to advertisement failed: %d\n", err); 
        return err;
    }

    err = bt_enable(NULL);
    if (err) 
    {
        printk("BT Enable Failed: %d\n", err); 
        return err;
    }

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
    if (err) 
    {
        printk("BT Advertisement create Failed: %d\n", err); 
        return err;
    }
    
	err = bt_le_ext_adv_set_data(adv, ad, ad_count, NULL, 0);
    if (err) 
    {
        printk("BT Advertisement data set Failed: %d\n", err); 
        return err;
    }

    bt_gatt_cb_register(&gatt_callbacks);


    
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
    if (err) 
    {
        printk("BT default adversiment start Failed: %d\n", err); 
        return err;
    }

    initialized = true;
    return 0;    
}



int ble_add_service(bt_uuid_128 *service_uuid, bt_gatt_attr *attributes, size_t attributes_len)
{
    if (initialized) 
    {
        printk("BT You can create dynamic services before bt_le_ext_adv_start() only!\n");
        return EAGAIN;
    }
    else 
    {
        int err = 0;

        // Выделяем место под сервис
        if (dynamic_services_count == 0)
        {
            dynamic_services = (bt_gatt_service *)k_malloc(1 * sizeof(bt_gatt_service));
            if (dynamic_services == nullptr) 
            {
                printk("BT GATT service create Failed: %d\n", err);
                return ENOSR;
            }
        }
        else
        {
            dynamic_services = (bt_gatt_service *)k_realloc(dynamic_services, (dynamic_services_count+1) * sizeof(bt_gatt_service));
            if (dynamic_services == nullptr) 
            {
                printk("BT GATT service create Failed: %d\n", err);
                return ENOSR;
            }            
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
            printk("BT GATT service register Failed: %d\n", err);
            return err;
        }
        dynamic_services_count++;

        // Добавляем сервис в Adversiment
        ad = (bt_data *)k_realloc(ad, (ad_count+1) * sizeof(bt_data));
        if (ad == nullptr) 
        {
            printk("BT GATT service adding Failed: %d\n", err);
            return ENOSR;
        }
        ad_count++;

        ad[ad_count-1].type = BT_DATA_UUID128_SOME;
        ad[ad_count-1].data_len = BT_UUID_SIZE_128;
        ad[ad_count-1].data = service_uuid->val;

        return err;
    }
}