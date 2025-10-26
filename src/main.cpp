#include "main.h"
#include "ble_part.h"
#include "nvs_part.h"


static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn0_dev = GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);

/*
	TODO:
	1) Отображается статусы батареи штатными средствами Android/iOS/macOS - сами данные можно пока отправлять рандомные (главное, чтобы было видно что они меняются раз в 10 сек)	
	2) Устройство получает данные в характеристику WRITE (Сервис обмена данными) и выводит их в лог
	3) Устройство возвращает полученные данные в характеристику NOTIFY без каких либо изменений (тем самым протестируем передачу данных различной длина по кругу)	
	4) Лог
*/




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

static void _BT_NOTIFICATIONS(const struct bt_gatt_attr *attr, uint16_t value)
{
    printk("Notification %s\n", (value == BT_GATT_CCC_NOTIFY) ? "enabled" : "disabled");
}





static bt_uuid_128 UUID_MAIN_TASK_SERVICE;
static bt_uuid_128 UUID_MAIN_TASK_CHARACTERISTIC_IN;
static bt_uuid_128 UUID_MAIN_TASK_CHARACTERISTIC_OUT;

static struct bt_gatt_chrc MAIN_TASK_CHARACTERITIC_IN;
static struct bt_gatt_chrc MAIN_TASK_CHARACTERITIC_OUT;
static struct _bt_gatt_ccc MAIN_TASK_CCC;

static struct bt_gatt_attr MAIN_TASK_ATTRIBUTES[6];

// *******************************************************************
// >>> MAIN <<<
// *******************************************************************

void ble_prepare_main_task_service()
{
	get_uuid(0, &UUID_MAIN_TASK_SERVICE);
	get_uuid(1, &UUID_MAIN_TASK_CHARACTERISTIC_IN);
	get_uuid(2, &UUID_MAIN_TASK_CHARACTERISTIC_OUT);



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

	MAIN_TASK_CCC = BT_GATT_CCC_INITIALIZER(_BT_NOTIFICATIONS, NULL, NULL);



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


	ble_add_service(&UUID_MAIN_TASK_SERVICE, MAIN_TASK_ATTRIBUTES, ARRAY_SIZE(MAIN_TASK_ATTRIBUTES));
}


int main(void)
{
	gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT);
	gpio_pin_configure_dt(&btn0_dev, GPIO_INPUT);
	gpio_pin_set_dt(&led0_dev, 1);


	nvs_begin();
	ble_prepare_main_task_service();
	ble_begin();


		int err = 0;
	uint8_t count = 0;
	bool send_notification = false;
    while (1) 
    {
        if (gpio_pin_get_dt(&btn0_dev) && !send_notification) 
		{
			count++;

			if (ble_device) 
			{
				BT_MAIN_TASK_Out = count;
				err = bt_gatt_notify(ble_device, &MAIN_TASK_ATTRIBUTES[4], &BT_MAIN_TASK_Out, sizeof(BT_MAIN_TASK_Out));
				if (err != 0)
					printk("Notify Error: %i\n", err);
			}

			send_notification = true;
		}
		else if (!gpio_pin_get_dt(&btn0_dev))
			send_notification = false;
    }   		
}