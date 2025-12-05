/*
 * PROTOCOL.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "PROTOCOL.h"


K_HEAP_DEFINE(UniversalTransportHeap, UNIVERSAL_TRANSPORT_HEAP_SIZE);

K_HEAP_DEFINE(UniversalHeapRX, UNIVERSAL_RX_HEAP_SIZE);



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

int getUnusedBuffer(tUniversalMessageRX **ptrBuffer, tUniversalMessageRX *poolBuffers, uint16_t poolBuffersLen)
{
	for (uint8_t i = 0; i < poolBuffersLen; i++)
	{
		if (!poolBuffers[i].inUse)
		{
			poolBuffers[i].inUse = 1;
			*ptrBuffer = &poolBuffers[i];
			return 0;
		}
	}

	return -1;
}


int parseNextByte(uint8_t newByte, tParcingProcessData *context) 
{
	if (!context->buildPacket)
	{
		if (newByte == PROTOCOL_PREAMBLE)
		{
			context->buildPacket = 1;
			context->messageBuf->length = 0;
			context->messageBuf->inUse = 1;

			context->lenPayloadBuf = 0;
			context->crcValueBuf = 0;
		}
		else
			return 0;
	}

	context->messageBuf->data[context->messageBuf->length] = newByte;

	if (context->messageBuf->length == PROTOCOL_INDEX_PL_LEN_MSB) // LEN MSB
	{
		context->lenPayloadBuf = newByte;
		context->lenPayloadBuf <<= 8;
	}
	else if (context->messageBuf->length == PROTOCOL_INDEX_PL_LEN_LSB) // LEN LSB
	{
		context->lenPayloadBuf |= newByte;
		if (context->lenPayloadBuf > PROTOCOL_MAX_PACKET_LENGTH)
		{
			context->buildPacket = 0;
			context->messageBuf->inUse = 0;
			return -EMSGSIZE;
		}
	}
	else if (context->messageBuf->length >= PROTOCOL_INDEX_PL_START)
	{
		if (context->messageBuf->length >= context->lenPayloadBuf + PROTOCOL_INDEX_PL_START)
		{
			if (context->messageBuf->length == context->lenPayloadBuf + PROTOCOL_INDEX_PL_START) // End Mark
			{
				if (newByte != PROTOCOL_END_MARK)
				{
					context->buildPacket = 0;
					context->messageBuf->inUse = 0;
					return -EPROTO;
				}
			}
			else if (context->messageBuf->length == context->lenPayloadBuf + PROTOCOL_INDEX_PL_START + 1) // CRC MSB
			{
				context->crcValueBuf = newByte;
				context->crcValueBuf <<= 8;
			}
			else if (context->messageBuf->length == context->lenPayloadBuf + PROTOCOL_INDEX_PL_START + 2) // CRC LSB
			{
				context->crcValueBuf |= newByte;


				uint16_t crcCalculatedBuf = calculateCRC(context->messageBuf->data, PROTOCOL_INDEX_PL_START + context->lenPayloadBuf + 1);
				if (context->crcValueBuf != crcCalculatedBuf)
				{
					context->buildPacket = 0;
					context->messageBuf->inUse = 0;					
					return -EBADMSG;
				}

				// Конец
				context->messageBuf->length++;
				context->buildPacket = 0; 
				return context->messageBuf->length;
			}
		}
	}

	context->messageBuf->length++;
	return 0;
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

int protocolCheckMC(uint8_t MC) 
{ 
	uint8_t messageCodes[] = PROTOCOL_MSG_CODES;

	for (int i = 0; i < sizeof(messageCodes); i++) 
	{
		if (MC == messageCodes[i])
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
		if (protocolCheckMC(newByte) < 0)
			return -EPROTO;
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

int parseByte(uint8_t newByte, tParcingContext *context) 
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