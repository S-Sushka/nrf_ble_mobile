/*
 * PROTOCOL.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "PROTOCOL.h"


K_HEAP_DEFINE(UniversalTransportHeap, UNIVERSAL_TRANSPORT_HEAP_SIZE);



int heapFreeWithCheck(struct k_heap *heap, void *data) 
{
	if (heap == NULL || data == NULL)
	{
		return -EINVAL;
	}

	k_heap_free(heap, data);
	return 0;
}

uint16_t calculateCRC(uint8_t *data, uint16_t length)
{
	uint16_t crc = 0xFFFF;
  
	for (uint16_t i = 1; i < length; i++) 
	{
		crc ^= (uint16_t)data[i] << 8;
		
		for (uint8_t j = 0; j < 8; j++) 
		{
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc <<= 1;
			
		}
	}
  
  return crc;
}



// *******************************************************************
// Проверки заголовка
// *******************************************************************


int protocolCheckMT(uint8_t MT) 
{ 
	uint8_t messageTypes[] = PROTOCOL_MSG_TYPES;

	for (int i = 0; i < sizeof(messageTypes); i++) 
	{
		if (MT == messageTypes[i])
		{
			return 0; 
		}
	}
	
	return -EPROTO;
}

int protocolCheckPayloadLength(uint16_t length) 
{ 
	if (length <= PROTOCOL_MAX_PACKET_LENGTH)
	{
		return 0;
	}

	return -EMSGSIZE;
}



// *******************************************************************
// Парсинг
// *******************************************************************

int processByte(uint8_t newByte, tParcingContext *context) 
{
	context->rxPacket->data[context->rxPacket->length] = newByte;


	switch (context->rxPacket->length) // Проверки заголовка пакета
	{
	case PROTOCOL_INDEX_MSG_TYPE:
		if (protocolCheckMT(newByte) < 0)
			return -EPROTO;
		break;
	case PROTOCOL_INDEX_MSG_CODE:
		break;
	case PROTOCOL_INDEX_PL_LEN_MSB:
		context->payloadLength = newByte;
		context->payloadLength <<= 8;
		break;
	case PROTOCOL_INDEX_PL_LEN_LSB:
		context->payloadLength |= newByte;
		if (protocolCheckPayloadLength(context->payloadLength) < 0)
			return -EMSGSIZE;
		break;
	};

	if (context->rxPacket->length == PROTOCOL_INDEX_PL_LEN_LSB) // Расширяем буфер для данных пакета
	{
		context->rxPacket->data =
			k_heap_realloc(&UniversalTransportHeap, context->rxPacket->data, PROTOCOL_INDEX_PL_START+context->payloadLength+PROTOCOL_END_PART_SIZE, K_NO_WAIT);

		if (!context->rxPacket->data)
			return -ENOMEM;
	}

	if (context->rxPacket->length > PROTOCOL_INDEX_PL_START) // Проверки окончания пакета
	{
		if (context->rxPacket->length == PROTOCOL_INDEX_PL_START+context->payloadLength) // END MARK
		{
			if (newByte != PROTOCOL_END_MARK)
				return -EPROTO;
		}
		else if (context->rxPacket->length == PROTOCOL_INDEX_PL_START+context->payloadLength+1) // CRC MSB
		{
			context->checksumBuffer = newByte;
			context->checksumBuffer <<= 8;
		}
		else if (context->rxPacket->length == PROTOCOL_INDEX_PL_START+context->payloadLength+2) // CRC LSB
		{
			context->checksumBuffer |= newByte;

			uint16_t crcCalculatedBuf = calculateCRC(context->rxPacket->data, PROTOCOL_INDEX_PL_START+context->payloadLength+1);
			if (context->checksumBuffer != crcCalculatedBuf)
			{
				return -EBADMSG;
			}

			return ++context->rxPacket->length;
		}
	}

	context->rxPacket->length++;
	return 0;
}

int parseNextByte(uint8_t newByte, tParcingContext *context) 
{
	int err = 0;

	if (context->rxPacket == NULL) // Пропускаем мусор, ждём начала пакета
	{
		if (newByte == 0xBB) // Инициализируем структуру
		{
			context->rxPacket = k_heap_alloc(&UniversalTransportHeap, sizeof(tUniversalMessage), K_NO_WAIT);
			if (!context->rxPacket)
			{
				return -EAGAIN;
			}

			context->rxPacket->data = k_heap_alloc(&UniversalTransportHeap, PROTOCOL_INDEX_PL_START, K_NO_WAIT);
			if (!context->rxPacket->data)
			{
				return -ENOMEM;
			}			

			context->rxPacket->source = 0;
			context->rxPacket->length = 0;
		}
	}

	if (context->rxPacket) // Парсим
	{
		err = processByte(newByte, context);

		if (err < 0)
		{
			freeUniversalMessage(context->rxPacket);
			freeParcingContex(context);
		}
		
		return err;
	}

	return 0;
}



// *******************************************************************
// Очистки контекста
// *******************************************************************

void freeUniversalMessage(tUniversalMessage *packet) 
{
	if (packet)
	{
		heapFreeWithCheck(&UniversalTransportHeap, packet);

		if (packet->data)
			heapFreeWithCheck(&UniversalTransportHeap, packet->data);			
	}
}

void freeParcingContex(tParcingContext *context) 
{
	if (context)
	{		
		context->rxPacket = NULL;
		context->payloadLength = 0;
		context->checksumBuffer = 0;
	}
}