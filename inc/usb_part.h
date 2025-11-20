/*
 * usb_part.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */


#pragma once

#include <stdio.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>

#include "PROTOCOL.h"

int usb_begin();