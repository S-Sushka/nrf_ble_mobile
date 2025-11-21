/*
 * main.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/settings/settings.h>


#define THREAD_STACK_SIZE_MAIN  2048
#define THREAD_PRIORITY_MAIN    0