/*
 * parser.c
 * src
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#include "parser.h"

// DEBUG
void _DEBUG_printBuffer(uint8_t *data, uint16_t length) 
{
    printk(" --- PARSER DEBUG --- : Receive Packet: ");
    for (int i = 0; i < length; i++) 
    {
        if (data[i] >= 0x10)
            printk("%x ", data[i]);
        else
            printk("0%x ", data[i]);
    }
    printk("\n");
}
// DEBUG


K_MSGQ_DEFINE(parser_queue, sizeof(tUniversalMessageRX *), 5, 1);

void parser_thread()
{
    tUniversalMessageRX *pkt;   

    while (1) 
    {
        k_msgq_get(&parser_queue, &pkt, K_FOREVER);
        _DEBUG_printBuffer(pkt->data, pkt->length);
        pkt->inUse = 0;
    }
}
K_THREAD_DEFINE(parser_thread_id, 1024, parser_thread, NULL, NULL, NULL, 7, 0, 0);
