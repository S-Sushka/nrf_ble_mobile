/*
 * usb_part.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "usb_part.h"



const struct device *const uart_cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);

K_SEM_DEFINE(usb_rx_sem, 0, 1);

static void usb_rx_handler(const struct device *dev, void *user_data);
static void usb_timeout_handler(struct k_work *work);

K_WORK_DELAYABLE_DEFINE(usb_timeout_work, usb_timeout_handler);

static tParcingContext *resetContexByTimeout = NULL;
static uint16_t USB_RX_TIMEOUT = 0;


K_MSGQ_DEFINE(usb_queue_tx, sizeof(tUniversalMessage *), QUEUE_SIZE_USB_TX, 1);
extern struct k_msgq parser_queue;  // Очередь парсера



int usb_begin(uint16_t rx_timeout_ms) 
{
	int err = 0;

	// Инициализация USB 
    if (!device_is_ready(uart_cdc_dev)) 
	{
        SEGGER_RTT_printf(0, " --- USB CDC ERR --- :\tCDC ACM device not ready\n");
        return -ENODEV;
    }	

	err = usb_enable(NULL);
	if (err != 0) 
	{
		SEGGER_RTT_printf(0, " --- USB CDC ERR --- :\tFailed to enable USB\n");
		return err;
	}

	USB_RX_TIMEOUT = rx_timeout_ms;

	// Устанавливаем и включаем прерывание
	err = uart_irq_callback_set(uart_cdc_dev, usb_rx_handler);
	if (err != 0) 
	{
		SEGGER_RTT_printf(0, " --- USB CDC ERR --- :\tRX Callback set Failed\n");
		return err;
	}
	uart_irq_rx_enable(uart_cdc_dev);	

	SEGGER_RTT_printf(0, " --- USB CDC --- :  Successful Initialized!\n");
	return 0;
}



// *******************************************************************
// Коллбэки / Прерывания
// *******************************************************************

static void usb_rx_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

    if (uart_irq_update(dev) && uart_irq_rx_ready(dev)) 
	{
		k_sem_give(&usb_rx_sem);
	}	
}

static void usb_timeout_handler(struct k_work *work) 
{
	freeUniversalMessage(resetContexByTimeout->rxPacket);
	freeParcingContex(resetContexByTimeout);

	// SEGGER_RTT_printf(0, " --- USB DBG --- :  USB Packet RX Timeout\n");
}



// *******************************************************************
// Поток Интерфейса
// *******************************************************************

void usb_thread(void)
{	
	int err = 0;

	tUniversalMessage  	*txPacket;
	tParcingContext		rxPacketContext;

	freeParcingContex(&rxPacketContext);
	resetContexByTimeout = &rxPacketContext;


	uint8_t byteBuf;
    while (1) 
    {
		if (k_msgq_get(&usb_queue_tx, &txPacket, K_NO_WAIT) == 0) // TX
		{
			for (int i = 0; i < txPacket->length; i++)
				uart_poll_out(uart_cdc_dev, txPacket->data[i]);

			freeUniversalMessage(txPacket);
		}

		if (k_sem_take(&usb_rx_sem, K_NO_WAIT) == 0) // RX
		{
			while (uart_fifo_read(uart_cdc_dev, &byteBuf, 1))
			{
				k_work_reschedule(&usb_timeout_work, K_MSEC(USB_RX_TIMEOUT)); // Запускаем таймаут 

				err = parseNextByte(byteBuf, &rxPacketContext);

				if (err < 0) 
				{
					k_work_cancel_delayable(&usb_timeout_work); // Останавливаем ожидание таймаута

					switch (err)
					{
					case -EAGAIN:
						SEGGER_RTT_printf(0, " --- USB RX ERR --- : Allocate New Message Failed!\n");
						break;
					case -ENOMEM:
						SEGGER_RTT_printf(0, " --- USB RX ERR --- : Allocate Data For Message Failed!\n");
						break;
					case -EPROTO:
						SEGGER_RTT_printf(0, " --- USB RX ERR --- : Invalid Message!\n");
						break;
					case -EMSGSIZE:
						SEGGER_RTT_printf(0, " --- USB RX ERR --- : Too Long Message!\n");
						break;
					case -EBADMSG:
						SEGGER_RTT_printf(0, " --- USB RX ERR --- : Message CRC Error!\n");
						break;
					default:
						SEGGER_RTT_printf(0, " --- USB RX ERR --- : Parsing Error: %d\n", err);
						break;
					}	

					freeUniversalMessage(rxPacketContext.rxPacket);
					freeParcingContex(&rxPacketContext);
					continue;
				}


				if (err > 0)
				{
					k_work_cancel_delayable(&usb_timeout_work); // Останавливаем ожидание таймаута

					rxPacketContext.rxPacket->source = MESSAGE_SOURCE_USB;
					err = k_msgq_put(&parser_queue, &rxPacketContext.rxPacket, K_NO_WAIT);
					if (err != 0)
					{
						SEGGER_RTT_printf(0, " --- PARSER ERR --- :  Parser Queue Put from BLE Error: %d\n", err);
					}

					freeParcingContex(&rxPacketContext);
					continue;
				}
			}
		}

		k_yield();
    }   	
}

K_THREAD_DEFINE(usb_thread_id, THREAD_STACK_SIZE_USB, usb_thread, NULL, NULL, NULL,
		THREAD_PRIORITY_USB, 0, 0);
