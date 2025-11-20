/*
 * PROTOCOL.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "PROTOCOL.h"


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