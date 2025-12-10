/*
 * packet_build.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

#include "packet_build.h"



// *******************************************************************
// Сборка частей пакета
// *******************************************************************

void putBeetwenData(uint8_t *buf, uint16_t *offset, uint16_t data) 
{
    buf[1] = data & 0xFF; 
    data >>= 8;
    buf[0] = data & 0xFF; 

    *offset += 2;
}

void putPart(uint8_t *buf, uint16_t *offset, uint8_t *part, uint16_t size) 
{
    for (int i = 0; i < size; i++)
        buf[i] = part[i];

    *offset += size;
}


void put_TAG_SN(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[NVS_SIZE_FD_TAG_SN];
    factory_data_load_fd_tag_sn(part_buf);
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_TAG_MODEL(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[NVS_SIZE_FD_TAG_MODEL];
    factory_data_load_fd_tag_model(part_buf);
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_TAG_ANALOG_BOARD_ID(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[NVS_SIZE_FD_TAG_ANALOG_BOARD_ID];
    factory_data_load_fd_tag_analog_board_id(part_buf);
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_KCV_ISSUER_PUBLIC_KEY(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[] = { 0x88, 0x71, 0xB1, 0x28 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_KCV_DEVICE_CERTIFICATE(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[] = { 0x16, 0x7C, 0xB5, 0x20 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_KCV_DEVICE_HUK(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[] = { 0xDC, 0x95, 0xC0 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_KCV_PROFILES_ISSUER_PUBLIC_KEY(uint8_t *buf, uint16_t *offset)
{
    uint8_t part_buf[] = { 0x74, 0xF3, 0xD8, 0x84 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_GLOBAL_STATUS(uint8_t *buf, uint16_t *offset)
{
    uint8_t part_buf[] = { 0x00, 0x00, 0x00, 0x00 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_GLOBAL_LCS(uint8_t *buf, uint16_t *offset) 
{
    uint8_t part_buf[] = { 0x23 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_VER_MAIN_FIRMWARE(uint8_t *buf, uint16_t *offset)
{
    uint8_t part_buf[] = { 0x01, 0x01, 0x01 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_VER_MAIN_BOOTLOADER(uint8_t *buf, uint16_t *offset)
{
    uint8_t part_buf[] = { 0x01, 0x00, 0x00 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_VER_FMCU_FIRMWARE(uint8_t *buf, uint16_t *offset)
{
    uint8_t part_buf[] = { 0x01, 0x00, 0x01 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}

void put_VER_UPDATABLE_BOOTLOADER(uint8_t *buf, uint16_t *offset)
{
    uint8_t part_buf[] = { 0x02, 0x03, 0x00 };
    putPart(buf, offset, part_buf, sizeof(part_buf));
}



// *******************************************************************
// Сборка пакетов
// *******************************************************************

void buildGetDeviceInfo(tUniversalMessage *pkt)
{
    pkt->data[PROTOCOL_INDEX_PREAMBLE] = PROTOCOL_PREAMBLE;
    pkt->data[PROTOCOL_INDEX_MSG_TYPE] = PROTOCOL_MSG_TYPE_ANSWER;
    pkt->data[PROTOCOL_INDEX_MSG_CODE] = 0x02;
    pkt->data[PROTOCOL_INDEX_PL_LEN_MSB] = PROTOCOL_PL_SIZE_DEVICE_INFO >> 8;
    pkt->data[PROTOCOL_INDEX_PL_LEN_LSB] = PROTOCOL_PL_SIZE_DEVICE_INFO;

    uint16_t offset = PROTOCOL_INDEX_PL_START;

    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_1);
    put_TAG_SN(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_2);
    put_TAG_MODEL(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_3);
    put_TAG_ANALOG_BOARD_ID(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_4);
    put_KCV_ISSUER_PUBLIC_KEY(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_5);
    put_KCV_DEVICE_CERTIFICATE(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_6);
    put_KCV_DEVICE_HUK(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_7);
    put_KCV_PROFILES_ISSUER_PUBLIC_KEY(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_8);
    put_GLOBAL_STATUS(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_9);
    put_GLOBAL_LCS(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_10);
    put_VER_MAIN_FIRMWARE(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_11);
    put_VER_MAIN_BOOTLOADER(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_12);
    put_VER_FMCU_FIRMWARE(&pkt->data[offset], &offset);
    putBeetwenData(&pkt->data[offset], &offset, PROTOCOL_BEETWEN_DEVICE_INFO_13);
    put_VER_UPDATABLE_BOOTLOADER(&pkt->data[offset], &offset);
    
    pkt->data[offset] = PROTOCOL_END_MARK;

    uint16_t crcCalculatedBuf = calculateCRC(pkt->data, offset+1);
    pkt->data[offset+2] = crcCalculatedBuf;
    crcCalculatedBuf >>= 8;
    pkt->data[offset+1] = crcCalculatedBuf;
}
