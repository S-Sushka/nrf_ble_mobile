#include "main.h"
#include "ble_part.h"
#include "nvs_part.h"

static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);



//#define NVS_NEED_CONFIGURE // <-- Загружает настройки в NVS: сохраняет сервисов/характеристик UUID



// *******************************************************************
// Создание динамического сервиса MAIN TASK
// *******************************************************************

static bt_uuid_128 UUID_MAIN_TASK_SERVICE;
static bt_uuid_128 UUID_MAIN_TASK_CHARACTERISTIC_IN;
static bt_uuid_128 UUID_MAIN_TASK_CHARACTERISTIC_OUT;

static struct bt_gatt_chrc MAIN_TASK_CHARACTERITIC_IN;
static struct bt_gatt_chrc MAIN_TASK_CHARACTERITIC_OUT;
static struct _bt_gatt_ccc MAIN_TASK_CCC;

static struct bt_gatt_attr MAIN_TASK_ATTRIBUTES[6];


static uint8_t BT_MAIN_TASK_In;
static uint8_t BT_MAIN_TASK_Out;

static bool main_task_notify_out = false;


// Коллбэки
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
    if (offset != 0 || len != 1)
	{
		printk("Too big message! Maximum is 1 byte\n");
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

    BT_MAIN_TASK_In = *((uint8_t*)buf);
	printk("Main Task IN New Value: %d\n", BT_MAIN_TASK_In);

	BT_MAIN_TASK_Out = BT_MAIN_TASK_In;
	if (main_task_notify_out)
		bt_gatt_notify(ble_device, &MAIN_TASK_ATTRIBUTES[4], &BT_MAIN_TASK_Out, sizeof(BT_MAIN_TASK_Out));

    return len;
}

static void BT_NOTIFICATIONS_MAIN_TASK_OUT(const struct bt_gatt_attr *attr, uint16_t value)
{
    bool notify = (value == BT_GATT_CCC_NOTIFY) ? true : false;

    main_task_notify_out = notify;
	printk(" --- BLE Notification --- :  Main Task OUT Notification %s\n", notify ? "enabled" : "disabled");
}


// Создание и добавление сервиса
int ble_prepare_main_task_service()
{
	if (get_uuid(0, &UUID_MAIN_TASK_SERVICE)) 
		return -EFAULT;

	if (get_uuid(1, &UUID_MAIN_TASK_CHARACTERISTIC_IN)) 
		return -EFAULT;
	
	if (get_uuid(2, &UUID_MAIN_TASK_CHARACTERISTIC_OUT)) 
		return -EFAULT;	
	
	MAIN_TASK_CHARACTERITIC_IN =
		{
			.uuid = &UUID_MAIN_TASK_CHARACTERISTIC_IN.uuid,
			.value_handle = 0, /* Заполнится стеком */
			.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		};

	MAIN_TASK_CHARACTERITIC_OUT =
		{
			.uuid = &UUID_MAIN_TASK_CHARACTERISTIC_OUT.uuid,
			.value_handle = 0, /* Заполнится стеком */
			.properties = BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		};

	MAIN_TASK_CCC = BT_GATT_CCC_INITIALIZER(BT_NOTIFICATIONS_MAIN_TASK_OUT, NULL, NULL);


	struct bt_gatt_attr temp[6] =
		{
			BT_GATT_PRIMARY_SERVICE(&UUID_MAIN_TASK_SERVICE),

			// Характеристика IN
			BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
							  BT_GATT_PERM_READ,
							  bt_gatt_attr_read_chrc,
							  NULL,
							  &MAIN_TASK_CHARACTERITIC_IN),

			BT_GATT_ATTRIBUTE(MAIN_TASK_CHARACTERITIC_IN.uuid,
							  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
							  BT_MAIN_TASK_read_state, BT_MAIN_TASK_write_state, &BT_MAIN_TASK_In),

			// Характеристика OUT
			BT_GATT_ATTRIBUTE(BT_UUID_GATT_CHRC,
							  BT_GATT_PERM_READ,
							  bt_gatt_attr_read_chrc,
							  NULL,
							  &MAIN_TASK_CHARACTERITIC_OUT),

			BT_GATT_ATTRIBUTE(MAIN_TASK_CHARACTERITIC_OUT.uuid,
							  BT_GATT_PERM_READ,
							  BT_MAIN_TASK_read_state, BT_MAIN_TASK_write_state, &BT_MAIN_TASK_Out),

			// Настройка Notification
			BT_GATT_ATTRIBUTE(BT_UUID_GATT_CCC,
							  BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
							  bt_gatt_attr_read_ccc,
							  bt_gatt_attr_write_ccc,
							  &MAIN_TASK_CCC),
		};
    	memcpy(MAIN_TASK_ATTRIBUTES, temp, sizeof(temp));


	return ble_add_service(&UUID_MAIN_TASK_SERVICE, MAIN_TASK_ATTRIBUTES, ARRAY_SIZE(MAIN_TASK_ATTRIBUTES));
}



// *******************************************************************
// >>> MAIN <<<
// *******************************************************************

int main(void)
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

	ble_prepare_main_task_service();
	ble_begin();

	uint8_t battery_level = 0;
	uint8_t battery_level_status = 0;
    while (1) 
    {
		k_msleep(10000);

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
    }   	

	return 0;
}