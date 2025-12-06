/*
 * parser.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "parser.h"

#include "nvs_part.h"
#include "ble_part.h"
#include "usb_part.h"


K_MSGQ_DEFINE(parser_queue, sizeof(tUniversalMessage *), QUEUE_SIZE_PARSER, 1);


tUniversalMessage *parsePRCommand(tUniversalMessage *pkt);
tUniversalMessage *parseCommand(tUniversalMessage *pkt);
tUniversalMessage *parseCRISPCommand(tUniversalMessage *pkt);



// *******************************************************************
// Поток Парсера
// *******************************************************************

void parser_thread()
{
    int err = 0;

    tUniversalMessage *pkt_request;   
    tUniversalMessage *pkt_answer;
    

    while (1) 
    {
        k_msgq_get(&parser_queue, &pkt_request, K_FOREVER); // Принимаем
        _DEBUG_printBuffer("RX", pkt_request->data, pkt_request->length);

        switch (pkt_request->data[PROTOCOL_INDEX_MSG_TYPE]) // Обрабатываем
        {
        case PROTOCOL_MSG_TYPE_PR_COMMAND:
            pkt_answer = parsePRCommand(pkt_request);
        break;
        case PROTOCOL_MSG_TYPE_COMMAND:
            pkt_answer = parseCommand(pkt_request);
        break;
        case PROTOCOL_MSG_TYPE_CRISP_COMMAND:
            pkt_answer = parseCRISPCommand(pkt_request);
        break;                
        }

        if (pkt_answer) // Отвечаем
        {
            _DEBUG_printBuffer("TX", pkt_answer->data, pkt_answer->length);
            if (pkt_request->source == MESSAGE_SOURCE_USB)
            {
                err = k_msgq_put(&usb_queue_tx, &pkt_answer, K_NO_WAIT);
                if (err != 0)
                    SEGGER_RTT_printf(0, " --- USB TX ERR --- :  USB TX Queue put error: %d\n", err);
            }
            else
            {
                err = k_msgq_put(&ble_queue_tx, &pkt_answer, K_NO_WAIT);
                if (err != 0)
                    SEGGER_RTT_printf(0, " --- BLE TX ERR --- :  BLE TX Queue put error: %d\n", err);
            }
        }

        freeUniversalMessage(pkt_request);
    }
}

K_THREAD_DEFINE(parser_thread_id, THREAD_STACK_SIZE_PARSER, parser_thread, NULL, NULL, NULL, THREAD_PRIORITY_PARSER, 0, 0);



// *******************************************************************
// Функции обработки пакетов
// *******************************************************************

tUniversalMessage *parsePRCommand(tUniversalMessage *pkt) 
{
    tUniversalMessage *pkt_answer = k_heap_alloc(&UniversalTransportHeap, sizeof(tUniversalMessage), K_NO_WAIT);
    if (!pkt_answer)
    {
        SEGGER_RTT_printf(0, " --- PARSER ERR --- : Allocate Answer Message Failed!\n");
        return NULL;
    }

    pkt_answer->source = pkt->source;
    pkt_answer->length = pkt->length;
    pkt_answer->data = k_heap_alloc(&UniversalTransportHeap, pkt->length, K_NO_WAIT);
    if (!pkt_answer->data)
    {
        SEGGER_RTT_printf(0, " --- PARSER ERR --- : Allocate Data For Answer Message Failed!\n");
        return NULL;
    }

    for (int i = 0; i < pkt->length; i++)
        pkt_answer->data[i] = pkt->data[i];

    uint16_t crcCalculatedBuf = 0;
    uint16_t payloadLenBuf = 0;

    pkt_answer->data[PROTOCOL_INDEX_MSG_TYPE] = PROTOCOL_MSG_TYPE_PR_ANSWER;

    payloadLenBuf = pkt_answer->data[PROTOCOL_INDEX_PL_LEN_MSB];
    payloadLenBuf <<= 8;
    payloadLenBuf |= pkt_answer->data[PROTOCOL_INDEX_PL_LEN_LSB];

    crcCalculatedBuf = calculateCRC(pkt_answer->data, payloadLenBuf + PROTOCOL_INDEX_PL_START + 1);

    pkt_answer->data[payloadLenBuf + PROTOCOL_INDEX_PL_START + 2] = crcCalculatedBuf;
    crcCalculatedBuf >>= 8;
    pkt_answer->data[payloadLenBuf + PROTOCOL_INDEX_PL_START + 1] = crcCalculatedBuf;

    return pkt_answer;
}

tUniversalMessage *parseCommand(tUniversalMessage *pkt) 
{
    tUniversalMessage *pkt_answer = NULL;
    uint16_t answerSize = 0;

    switch (pkt->data[PROTOCOL_INDEX_MSG_CODE])
    {
    case PROTOCOL_MSG_CODE_GET_DEVICE_INFO:
        answerSize = PROTOCOL_INDEX_PL_START + PROTOCOL_PL_SIZE_DEVICE_INFO + PROTOCOL_END_PART_SIZE;

        pkt_answer = k_heap_alloc(&UniversalTransportHeap, sizeof(tUniversalMessage), K_NO_WAIT);

        pkt_answer->source = pkt->source;
        pkt_answer->length = answerSize;        
        pkt_answer->data = k_heap_alloc(&UniversalTransportHeap, answerSize, K_NO_WAIT);

        uint8_t sn_buf[NVS_SIZE_FD_TAG_SN];
        uint8_t md_buf[NVS_SIZE_FD_TAG_MODEL];
        uint8_t id_buf[NVS_SIZE_FD_TAG_ANALOG_BOARD_ID];

        factory_data_load_fd_tag_sn(sn_buf);
        factory_data_load_fd_tag_model(md_buf);
        factory_data_load_fd_tag_analog_board_id(id_buf);

        SEGGER_RTT_printf(0, "SN: %x %x %x %x\n", sn_buf[0], sn_buf[1], sn_buf[2], sn_buf[3]);
        SEGGER_RTT_printf(0, "MODEL: %x %x %x\n", md_buf[0], md_buf[1], md_buf[2]);
        SEGGER_RTT_printf(0, "ID: %x\n", id_buf[0]);

    break;
    }

    return pkt_answer;
}

tUniversalMessage *parseCRISPCommand(tUniversalMessage *pkt) 
{
    return NULL;
}



// *******************************************************************
// Отладка
// *******************************************************************

void _DEBUG_printBuffer(const char *prefix, uint8_t *data, uint16_t length) 
{
    SEGGER_RTT_printf(0, " --- %s PACKET --- : ", prefix);
    for (int i = 0; i < length; i++) 
    {
        if (data[i] >= 0x10)
            SEGGER_RTT_printf(0, "%x ", data[i]);
        else
            SEGGER_RTT_printf(0, "0%x ", data[i]);
    }
    SEGGER_RTT_printf(0, "\n");
}
