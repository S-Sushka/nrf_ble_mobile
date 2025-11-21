/*
 * usb_part.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "usb_part.h"


const struct device *const uart_cdc_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);


static tUniversalMessageRX poolBuffersRX_USB[COUNT_USB_RX_POOL_BUFFERS];  // Буферы приёма
static tUniversalMessageRX *currentPoolBufferRX_USB = NULL;               // Текущий Буфер

extern struct k_msgq parser_queue;  // Очередь парсера



static void interrupt_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

    static uint8_t byteBuf;

    static uint16_t lenPayloadBuf;
    static uint16_t crcValueBuf;

    static uint8_t buildPacket = 0;

    int err = 0; 


    // Проверяем, есть ли событие приёма
    if (uart_irq_update(dev) && uart_irq_rx_ready(dev)) 
	{
        while (uart_fifo_read(dev, &byteBuf, 1)) 
		{
				if (!buildPacket) 
				{
					if (byteBuf == PROTOCOL_PREAMBLE)
					{
						if (getUnusedBuffer(&currentPoolBufferRX_USB, poolBuffersRX_USB, COUNT_BLE_RX_POOL_BUFFERS) == 0)
						{
							buildPacket = 1;
							currentPoolBufferRX_USB->length = 0;
							currentPoolBufferRX_USB->inUse = 1;
						}
						else 
							printk(" --- USB ERR --- : There are no free buffers!\n");
					}
					else
						continue;
				}

				currentPoolBufferRX_USB->data[ currentPoolBufferRX_USB->length ] = byteBuf;

				if (currentPoolBufferRX_USB->length == PROTOCOL_INDEX_PL_LEN) // LEN MSB
				{
					lenPayloadBuf = byteBuf;
					lenPayloadBuf <<= 8;
				}
				else if (currentPoolBufferRX_USB->length == PROTOCOL_INDEX_PL_LEN + 1) // LEN LSB
				{
					lenPayloadBuf |= byteBuf;
				}
				else if (currentPoolBufferRX_USB->length > PROTOCOL_INDEX_PL_LEN + 1)
				{
					if (currentPoolBufferRX_USB->length >= lenPayloadBuf + PROTOCOL_INDEX_PL_START) 
					{
						if (currentPoolBufferRX_USB->length == lenPayloadBuf+PROTOCOL_INDEX_PL_START) // End Mark
						{
							if (byteBuf != PROTOCOL_END_MARK) 
								printk(" --- USB ERR --- :  Bad END MARK byte for recieved Packet!\n");
						}
						else if (currentPoolBufferRX_USB->length == lenPayloadBuf+PROTOCOL_INDEX_PL_START+1) // CRC MSB 
						{
							crcValueBuf = byteBuf;
							crcValueBuf <<= 8;
						}
						else if (currentPoolBufferRX_USB->length == lenPayloadBuf+PROTOCOL_INDEX_PL_START+2) // CRC LSB 
						{
							crcValueBuf |= byteBuf;

							uint16_t crcCalculatedBuf = calculateCRC(currentPoolBufferRX_USB->data, lenPayloadBuf+PROTOCOL_INDEX_PL_START+1);

							if (crcValueBuf != crcCalculatedBuf) 
								printk(" --- USB ERR --- :  Bad CRC for recieved Packet!\n");


							currentPoolBufferRX_USB->source = MESSAGE_SOURCE_USB;
							err = k_msgq_put(&parser_queue, &currentPoolBufferRX_USB, K_NO_WAIT);
							if (err != 0)
								printk(" --- PARSER ERR --- :  Parser queue put error: %d\n", err);

							buildPacket = 0; // Конец
						}                
					}
				}

				currentPoolBufferRX_USB->length++;
        }
    }	
}


int usb_begin() 
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

	// Устанавливаем и включаем прерывание
	err = uart_irq_callback_set(uart_cdc_dev, interrupt_handler);
	if (err != 0) 
	{
		printk(" --- USB CDC ERR --- :\tRX Callback set Failed\n");
		return err;
	}
	uart_irq_rx_enable(uart_cdc_dev);


	printk(" --- USB CDC --- :  Successful Initialized!\n");
	return 0;
}



// *******************************************************************
// Поток Интерфейса
// *******************************************************************

K_MSGQ_DEFINE(usb_queue_tx, sizeof(tUniversalMessageTX), 4, 1);

void usb_thread(void)
{	
	tUniversalMessageTX pkt;

    while (1) 
    {
		k_msgq_get(&usb_queue_tx, &pkt, K_FOREVER);
		for (int i = 0; i < pkt.length; i++)
			uart_poll_out(uart_cdc_dev, pkt.data[i]);
    }   	
}

K_THREAD_DEFINE(usb_thread_id, THREAD_STACK_SIZE_USB, usb_thread, NULL, NULL, NULL,
		THREAD_PRIORITY_USB, 0, 0);
