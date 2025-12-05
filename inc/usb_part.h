/*
 * usb_part.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#pragma once

#include <SEGGER_RTT.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "PROTOCOL.h"



#define THREAD_STACK_SIZE_USB   16384
#define THREAD_PRIORITY_USB     7


extern struct k_msgq usb_queue_tx;  // Очередь отправки пакетов по USB

int usb_begin(uint16_t rx_timeout_ms);