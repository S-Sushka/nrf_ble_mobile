#include "main.h"
#include "ble_part.h"
#include "nvs_part.h"


static const struct gpio_dt_spec led0_dev = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec btn0_dev = GPIO_DT_SPEC_GET(DT_NODELABEL(button0), gpios);


// *******************************************************************
// >>> MAIN <<<
// *******************************************************************


void prepare_ble_main_task_service() 
{
	static bt_uuid_128 UUID_MAIN_TASK_SERVICE;
	static bt_uuid_128 UUID_MAIN_TASK_CHARACTERISTIC_IN;
	static bt_uuid_128 UUID_MAIN_TASK_CHARACTERISTIC_OUT;

	get_uuid(0, &UUID_MAIN_TASK_SERVICE);
	get_uuid(1, &UUID_MAIN_TASK_CHARACTERISTIC_IN);
	get_uuid(2, &UUID_MAIN_TASK_CHARACTERISTIC_OUT);
	ble_create_main_task_service(&UUID_MAIN_TASK_SERVICE, &UUID_MAIN_TASK_CHARACTERISTIC_IN, &UUID_MAIN_TASK_CHARACTERISTIC_OUT);	
}

int main(void)
{
	gpio_pin_configure_dt(&led0_dev, GPIO_OUTPUT);
	gpio_pin_configure_dt(&btn0_dev, GPIO_INPUT);
	gpio_pin_set_dt(&led0_dev, 1);


	nvs_begin();
	prepare_ble_main_task_service();
	ble_begin();


	uint8_t count = 0;
	bool send_notification = false;
    while (1) 
    {
        if (gpio_pin_get_dt(&btn0_dev) && !send_notification) 
		{
			count++;
			ble_test_notify(count);
			send_notification = true;
		}
		else if (!gpio_pin_get_dt(&btn0_dev))
			send_notification = false;

		ble_test_mtu();
    }   		
}