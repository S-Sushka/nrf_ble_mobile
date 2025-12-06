/*
 * main.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

#pragma once

#include <SEGGER_RTT.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/settings/settings.h>



#define THREAD_STACK_SIZE_MAIN  16384
#define THREAD_PRIORITY_MAIN    0