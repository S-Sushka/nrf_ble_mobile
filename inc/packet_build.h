/*
 * packet_build.h
 * inc
 * 
 * Copyright (C) 2025 by UHF Solutions
 * All right reserved
 */

 #pragma once

 #include <zephyr/kernel.h>
 
 #include "PROTOCOL.h"
 #include "nvs_part.h"

 
void buildGetDeviceInfo(tUniversalMessage *pkt);