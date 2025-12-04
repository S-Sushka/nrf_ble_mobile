/*
 * parser.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "parser.h"

#include "ble_part.h"
#include "usb_part.h"




K_MSGQ_DEFINE(parser_queue, sizeof(tUniversalMessageRX *), 4, 1);

void parser_thread()
{
    int err = 0;

    tUniversalMessageRX *pkt;   

    tUniversalMessageTX pkt_echo;
    uint8_t data_echo[PROTOCOL_MAX_PACKET_LENGTH];


    pkt_echo.data = data_echo;
    while (1) 
    {
        k_msgq_get(&parser_queue, &pkt, K_FOREVER);
        _DEBUG_printBuffer("RX", pkt->data, pkt->length);

        if (pkt->data[PROTOCOL_INDEX_MT] == PROTOCOL_MSG_TYPE_PR_COMMAND) 
        {
            pkt_echo.source = pkt->source;
            pkt_echo.length = pkt->length;

            SEGGER_RTT_printf(0, "PACKET LEN: %d\n", pkt_echo.length);

            for (int i = 0; i < pkt->length; i++) 
                data_echo[i] = pkt->data[i];

            uint16_t crcCalculatedBuf = 0;
            uint16_t payloadLenBuf = 0;


            pkt_echo.data[PROTOCOL_INDEX_MT] = PROTOCOL_MSG_TYPE_PR_ANSWER;

            payloadLenBuf = pkt_echo.data[PROTOCOL_INDEX_PL_LEN];
            payloadLenBuf <<= 8;
            payloadLenBuf |= pkt_echo.data[PROTOCOL_INDEX_PL_LEN+1];

            crcCalculatedBuf = calculateCRC(pkt_echo.data, payloadLenBuf+PROTOCOL_INDEX_PL_START+1);

            pkt_echo.data[payloadLenBuf+PROTOCOL_INDEX_PL_START+2] = crcCalculatedBuf;
            crcCalculatedBuf >>= 8;
            pkt_echo.data[payloadLenBuf+PROTOCOL_INDEX_PL_START+1] = crcCalculatedBuf;
            

            _DEBUG_printBuffer("TX", pkt_echo.data, pkt_echo.length);
            if (pkt->source == MESSAGE_SOURCE_USB)
            {
                err = k_msgq_put(&usb_queue_tx, &pkt_echo, K_NO_WAIT);
                if (err != 0)
                    SEGGER_RTT_printf(0, " --- USB TX ERR --- :  USB TX Queue put error: %d\n", err);
            }
            else
            {
                err = k_msgq_put(&ble_queue_tx, &pkt_echo, K_NO_WAIT);
                if (err != 0)
                    SEGGER_RTT_printf(0, " --- BLE TX ERR --- :  BLE TX Queue put error: %d\n", err);
            }
        }

        k_heap_free(&UniversalHeapRX, pkt->data);
        pkt->inUse = 0;
    }
}
K_THREAD_DEFINE(parser_thread_id, THREAD_STACK_SIZE_PARSER, parser_thread, NULL, NULL, NULL, THREAD_PRIORITY_PARSER, 0, 0);


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
