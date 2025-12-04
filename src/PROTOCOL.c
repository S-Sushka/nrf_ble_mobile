/*
 * PROTOCOL.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "PROTOCOL.h"

K_HEAP_DEFINE(UniversalHeapRX, UNIVERSAL_RX_HEAP_SIZE);



int heapFreeWithCheck(struct k_heap *heap, void *data) 
{
	if (!heap || !data)
		return -EINVAL;

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
		}
		else
			return 0;
	}

	context->messageBuf->data[context->messageBuf->length] = newByte;

	if (context->messageBuf->length == PROTOCOL_INDEX_PL_LEN) // LEN MSB
	{
		context->lenPayloadBuf = newByte;
		context->lenPayloadBuf <<= 8;
	}
	else if (context->messageBuf->length == PROTOCOL_INDEX_PL_LEN + 1) // LEN LSB
	{
		context->lenPayloadBuf |= newByte;
		if (context->lenPayloadBuf > PROTOCOL_MAX_PACKET_LENGTH)
		{
			context->buildPacket = 0;
			context->messageBuf->inUse = 0;
			return -EMSGSIZE;
		}
	}
	else if (context->messageBuf->length > PROTOCOL_INDEX_PL_LEN + 1)
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