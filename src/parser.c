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
    uint8_t data_echo[MESSAGE_BUFFER_SIZE];


    pkt_echo.data = data_echo;
    while (1) 
    {
        k_msgq_get(&parser_queue, &pkt, K_FOREVER);
        _DEBUG_printBuffer("RX", pkt->data, pkt->length);

        if (pkt->data[PROTOCOL_INDEX_MT] == PROTOCOL_MSG_TYPE_PR_COMMAND) 
        {
            pkt_echo.source = pkt->source;
            pkt_echo.length = pkt->length;
            for (int i = 0; i < pkt->length; i++) 
                data_echo[i] = pkt->data[i];

            uint16_t crcCalculatedBuf = 0;
            uint16_t payloadLenBuf = 0;


            pkt_echo.data[PROTOCOL_INDEX_MT] = PROTOCOL_MSG_TYPE_PR_ANSWER;

            payloadLenBuf = pkt_echo.data[PROTOCOL_INDEX_PL_LEN];
            payloadLenBuf <<= 8;
            payloadLenBuf |= pkt_echo.data[PROTOCOL_INDEX_PL_LEN+1];

            crcCalculatedBuf = calculateCRC(pkt_echo.data, payloadLenBuf+PROTOCOL_INDEX_PL_START+1);

            pkt_echo.data[payloadLenBuf+PROTOCOL_INDEX_PL_START+1] = crcCalculatedBuf & 0x00FF;
            crcCalculatedBuf >>= 8;
            pkt_echo.data[payloadLenBuf+PROTOCOL_INDEX_PL_START+2] = crcCalculatedBuf;
            

            _DEBUG_printBuffer("TX", pkt_echo.data, pkt_echo.length);
            if (pkt->source == MESSAGE_SOURCE_USB)
            {
                err = k_msgq_put(&usb_queue_tx, &pkt_echo, K_NO_WAIT);
                if (err != 0)
                    printk(" --- USB TX ERR --- :  USB TX Queue put error: %d\n", err);
            }
            else
            {
                err = k_msgq_put(&ble_queue_tx, &pkt_echo, K_NO_WAIT);
                if (err != 0)
                    printk(" --- BLE TX ERR --- :  BLE TX Queue put error: %d\n", err);
            }
        }

        pkt->inUse = 0;
    }
}
K_THREAD_DEFINE(parser_thread_id, 2048, parser_thread, NULL, NULL, NULL, 7, 0, 0);


void _DEBUG_printBuffer(const char *prefix, uint8_t *data, uint16_t length) 
{
    printk(" --- %s PACKET --- : ", prefix);
    for (int i = 0; i < length; i++) 
    {
        if (data[i] >= 0x10)
            printk("%x ", data[i]);
        else
            printk("0%x ", data[i]);
    }
    printk("\n");
}
