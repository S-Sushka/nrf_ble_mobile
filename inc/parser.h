/*
 * parser.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

#pragma once

#include <SEGGER_RTT.h>

#include <zephyr/kernel.h>
#include "PROTOCOL.h"



#define THREAD_STACK_SIZE_PARSER   16384
#define THREAD_PRIORITY_PARSER     7


extern struct k_msgq parser_queue;  // Очередь парсера


void _DEBUG_printBuffer(const char *prefix, uint8_t *data, uint16_t length);
