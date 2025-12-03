/*
 * PROTOCOL.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#pragma once

#include <stdint.h>
#include <zephyr/kernel.h>


#define MAX_PL_PACKET_LENGTH		4096

#define MESSAGE_BUFFER_SIZE			5000 	// Размер для буферов обмена
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

	uint8_t *data;						// Указатель на буфер принимаемого сообщения
	uint16_t length;    				// Длина принимаемого сообщения

	uint8_t inUse;						// Флаг занятости буфера. После того, как обработали данные, записываем 0
} tUniversalMessageRX;



#define PROTOCOL_PREAMBLE 0xBB
#define PROTOCOL_END_MARK 0x7E


#define PROTOCOL_INDEX_PREAMBLE 0
#define PROTOCOL_INDEX_MT		1
#define PROTOCOL_INDEX_MC		2
#define PROTOCOL_INDEX_PL_LEN	3
#define PROTOCOL_INDEX_PL_START	5


#define PROTOCOL_MSG_TYPE_PR_COMMAND	0x00
#define PROTOCOL_MSG_TYPE_PR_ANSWER		0x01
#define PROTOCOL_MSG_TYPE_PR_NOTIFY		0x02

#define PROTOCOL_MSG_TYPE_COMMAND		0xE0
#define PROTOCOL_MSG_TYPE_ANSWER		0xE1
#define PROTOCOL_MSG_TYPE_NOTIFY		0xE2

#define PROTOCOL_MSG_TYPE_CRISP_COMMAND	0xE5
#define PROTOCOL_MSG_TYPE_CRISP_ANSWER	0xE6
#define PROTOCOL_MSG_TYPE_CRISP_NOTIFY	0xE7


#define PROTOCOL_MSG_CODE_GET_DEVICE_INFO		0x01
#define PROTOCOL_MSG_CODE_DO_SELF_CHECK			0x02
#define PROTOCOL_MSG_CODE_GET_ALL_USR_SETTINGS	0x10
#define PROTOCOL_MSG_CODE_GET_USR_SETTING		0x11
#define PROTOCOL_MSG_CODE_SET_USR_SETTING		0x12
#define PROTOCOL_MSG_CODE_GET_BATTERY_INFO		0x13
#define PROTOCOL_MSG_CODE_SET_SYSTEM_MODE		0x20
#define PROTOCOL_MSG_CODE_SET_SYSTEM_REG		0xF0
#define PROTOCOL_MSG_CODE_SET_LIFE_CYCLE		0xF1



uint16_t calculateCRC(uint8_t *data, uint16_t length);
int getUnusedBuffer(tUniversalMessageRX **ptrBuffer, tUniversalMessageRX *poolBuffers, uint16_t poolBuffersLen);


typedef struct 
{
	tUniversalMessageRX *messageBuf;

    uint16_t lenPayloadBuf;
    uint16_t crcValueBuf;

    uint8_t buildPacket;
} tParcingProcessData;

/*
	Универсальная функция для парсинга приходящих пакетов. 
	Принимает новый	пришедший байт и контекст для каждого конкретного пакета 
	Возвращает <0 - если ошибка; 0 - если байт запарсен успешно; >0 - длина, если пакет запарсен успешно
*/

int parseNextByte(uint8_t newByte, tParcingProcessData *context);