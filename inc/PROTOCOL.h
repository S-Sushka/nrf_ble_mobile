/*
 * PROTOCOL.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#pragma once

#include <stdint.h>


#define MESSAGE_RX_BUFFER_SIZE		512 	// Размер одного буфера принимаемого сообщения
#define COUNT_BLE_RX_POOL_BUFFERS	4
#define COUNT_USB_RX_POOL_BUFFERS	4

typedef struct
{
	uint8_t *data;  	// Указатель на передаваемое сообщение 
	uint16_t length;	// Длина передаваемого сообщения
} tUniversalMessageTX;

typedef struct
{
	uint8_t data[MESSAGE_RX_BUFFER_SIZE];	// Буфер для принимаемого сообщения
	uint16_t length;    					// Длина принимаемого сообщения
	uint8_t inUse;							// Флаг занятости буфера. После того, как обработали данные, записываем 0
} tUniversalMessageRX;


#define PROTOCOL_PREAMBLE 0xBB
#define PROTOCOL_END_MARK 0x7E

#define PROTOCOL_INDEX_HEAD_LENGTH	5
#define PROTOCOL_INDEX_PL_LEN_START 3



uint16_t calculateCRC(uint8_t *data, uint16_t length);  // End Mark учитывается, Преамбула НЕ учитывается
int getUnusedBuffer(tUniversalMessageRX **ptrBuffer, tUniversalMessageRX *poolBuffers, uint16_t poolBuffersLen);