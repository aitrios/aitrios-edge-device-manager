/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "led_manager/config/config.h"

// Power Led priority table.
static const EsfLedManagerPriority kPowerLedControlTable[] = { };

// Wifi Led priority table.
static const EsfLedManagerPriority kWifiLedControlTable[] = { };

// Service priority table.
static const EsfLedManagerPriority kServiceLedControlTable[] = { };

// LED priority table.
LED_MANAGER_STATIC LED_MANAGER_CONSTANT struct EsfLedManagerLedTables
    kLedControlTable[kEsfLedManagerTargetLedNum] = {
        ESF_LED_MANAGER_SET_TABLE(kPowerLedControlTable),
        ESF_LED_MANAGER_SET_TABLE(kWifiLedControlTable),
        ESF_LED_MANAGER_SET_TABLE(kServiceLedControlTable),
};
