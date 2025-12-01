/*
 * usb_part.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "usb_part.h"



const struct device *const uart_cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);


static void usb_rx_handler(const struct device *dev, void *user_data);
static void usb_timeout_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(usb_timeout_work, usb_timeout_handler);


static uint16_t USB_RX_TIMEOUT;

static tUniversalMessageRX poolBuffersRX_USB[COUNT_BLE_RX_POOL_BUFFERS];                // Буферы приёма
static uint8_t dataPoolBuffersRX_USB[COUNT_BLE_RX_POOL_BUFFERS][MESSAGE_BUFFER_SIZE];	// Данные буферов приёма
static tUniversalMessageRX *currentPoolBufferRX_USB = NULL;                				// Текущий буфер

static tParcingProcessData context_USB; // Контекст для выполнения парсинга

extern struct k_msgq parser_queue;  // Очередь парсера



int usb_begin(uint16_t rx_timeout_ms) 
{
	int err = 0;

	// Инициализация USB 
    if (!device_is_ready(uart_cdc_dev)) 
	{
        printk(" --- USB CDC ERR --- :\tCDC ACM device not ready\n");
        return -ENODEV;
    }	

	err = usb_enable(NULL);
	if (err != 0) 
	{
		printk(" --- USB CDC ERR --- :\tFailed to enable USB\n");
		return err;
	}

	USB_RX_TIMEOUT = rx_timeout_ms;

	// Устанавливаем и включаем прерывание
	err = uart_irq_callback_set(uart_cdc_dev, usb_rx_handler);
	if (err != 0) 
	{
		printk(" --- USB CDC ERR --- :\tRX Callback set Failed\n");
		return err;
	}
	uart_irq_rx_enable(uart_cdc_dev);


	printk(" --- USB CDC --- :  Successful Initialized!\n");
	return 0;
}


static void usb_rx_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

    static uint8_t byteBuf;

    int err = 0; 


    // Проверяем, есть ли событие приёма
    if (uart_irq_update(dev) && uart_irq_rx_ready(dev)) 
	{

        while (uart_fifo_read(dev, &byteBuf, 1)) 
		{
			if (!context_USB.buildPacket) 
			{
				if (byteBuf == PROTOCOL_PREAMBLE)
				{
					if (getUnusedBuffer(&currentPoolBufferRX_USB, poolBuffersRX_USB, COUNT_BLE_RX_POOL_BUFFERS) == 0)
					{
						currentPoolBufferRX_USB->source = MESSAGE_SOURCE_USB;
						currentPoolBufferRX_USB->length = 0;
						currentPoolBufferRX_USB->inUse = 1;

						context_USB.messageBuf = currentPoolBufferRX_USB;
						context_USB.buildPacket = 1;    
					}
					else
					{
						printk(" --- USB ERR --- : There are no free buffers!\n");
						break;
					}
				}
				else
					continue;				
			}

			err = parseNextByte(byteBuf, &context_USB);
			if (err < 0)
			{
				switch (err)
				{
				case -EMSGSIZE:
					printk(" --- USB ERR --- :  Recieve Packet too Long!\n");
					break;
				case -EPROTO:
					printk(" --- USB ERR --- :  Bad END MARK byte for recieved Packet!\n");
					break;
				case -EBADMSG:
					printk(" --- USB ERR --- :  Bad CRC for recieved Packet!\n");
					break;
				default:
					printk(" --- USB ERR --- :  Parse Packet Error: %d\n", err);
					break;
				}
				break;
			}
			else if (err > 0)
			{
				err = k_msgq_put(&parser_queue, &currentPoolBufferRX_USB, K_NO_WAIT);
				if (err != 0)
				{
					printk(" --- PARSER ERR --- :  Parser queue put error: %d\n", err);
					break;
				}
			}

			k_work_schedule(&usb_timeout_work, K_MSEC(USB_RX_TIMEOUT)); // Запускаем ожидание таймаута
   		}

		k_work_cancel_delayable(&usb_timeout_work); // Останавливаем ожидание таймаута
	}	
}

static void usb_timeout_handler(struct k_work *work) 
{
	if (currentPoolBufferRX_USB) 
	{
		currentPoolBufferRX_USB->length = 0;
		currentPoolBufferRX_USB->inUse = 0;
	}
	context_USB.buildPacket = 0; 

	// printk(" --- USB DBG --- :  USB Packet RX Timeout\n");
}


// *******************************************************************
// Поток Интерфейса
// *******************************************************************

K_MSGQ_DEFINE(usb_queue_tx, sizeof(tUniversalMessageTX), 4, 1);

void usb_thread(void)
{	
	tUniversalMessageTX pkt;

	for (uint16_t i = 0; i < COUNT_USB_RX_POOL_BUFFERS; i++) 
	{
		poolBuffersRX_USB[i].data = dataPoolBuffersRX_USB[i];
	}

    while (1) 
    {
		k_msgq_get(&usb_queue_tx, &pkt, K_FOREVER);
		for (int i = 0; i < pkt.length; i++)
			uart_poll_out(uart_cdc_dev, pkt.data[i]);
    }   	
}

K_THREAD_DEFINE(usb_thread_id, THREAD_STACK_SIZE_USB, usb_thread, NULL, NULL, NULL,
		THREAD_PRIORITY_USB, 0, 0);
