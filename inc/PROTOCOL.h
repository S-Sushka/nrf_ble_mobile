/*
 * PROTOCOL.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#pragma once

#include <stdint.h>


#define MESSAGE_BUFFER_SIZE			512 	// Размер для буферов обмена
#define COUNT_BLE_RX_POOL_BUFFERS	4
#define COUNT_USB_RX_POOL_BUFFERS	4



typedef enum 
{
	MESSAGE_SOURCE_USB = 0,
	MESSAGE_SOURCE_BLE_CONNS,
} tMessageSources;

typedef struct
{
	tMessageSources source; // Номер источника. Всё, что >= MESSAGE_SOURCE_BLE_CONNS - индексы BLE подключений

	uint8_t *data;  		// Указатель на передаваемое сообщение 
	uint16_t length;		// Длина передаваемого сообщения
} tUniversalMessageTX;

typedef struct
{
	tMessageSources source; 			// Номер источника. Всё, что >= MESSAGE_SOURCE_BLE_CONNS - индексы BLE подключений

	uint8_t data[MESSAGE_BUFFER_SIZE];	// Буфер для принимаемого сообщения
	uint16_t length;    				// Длина принимаемого сообщения

	uint8_t inUse;						// Флаг занятости буфера. После того, как обработали данные, записываем 0
} tUniversalMessageRX;



#define PROTOCOL_PREAMBLE 0xBB
#define PROTOCOL_END_MARK 0x7E

#define PROTOCOL_MT_COMMAND	0x00
#define PROTOCOL_MT_ANSWER	0x01
#define PROTOCOL_MT_NOTIFY	0x02

#define PROTOCOL_INDEX_PREAMBLE 0
#define PROTOCOL_INDEX_MT		1
#define PROTOCOL_INDEX_MC		2
#define PROTOCOL_INDEX_PL_LEN	3
#define PROTOCOL_INDEX_PL_START	5



uint16_t calculateCRC(uint8_t *data, uint16_t length);  // End Mark учитывается, Преамбула НЕ учитывается
int getUnusedBuffer(tUniversalMessageRX **ptrBuffer, tUniversalMessageRX *poolBuffers, uint16_t poolBuffersLen);