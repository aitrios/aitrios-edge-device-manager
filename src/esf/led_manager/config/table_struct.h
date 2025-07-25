/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_LED_MANAGER_CONFIG_TABLE_STRUCT_H_
#define ESF_LED_MANAGER_CONFIG_TABLE_STRUCT_H_

#include <stdint.h>
#include <stddef.h>

#include "led_manager/include/led_manager.h"

#define ESF_LED_MANAGER_SET_TABLE(table) \
  {(table), sizeof(table) / sizeof(*(table))}

#define ESF_LED_MANAGER_SET_TABLE_EMPTY {NULL, 0}

// This is an enumerated type of LED lighting cycle.
typedef enum EsfLedManagerLedInterval {
  kEsfLedManagerLedIntervalNone,     // Lighting.
  kEsfLedManagerLedInterval1Hz,      // 1Hz Blinking.
  kEsfLedManagerLedInterval4Hz,      // 4Hz Blinking.
  kEsfLedManagerLedIntervalPattern,  // Pattern Blinking.
  kEsfLedManagerLedIntervalNum       // The number of this Enum value.
} EsfLedManagerLedInterval;

// This structure stores the state information necessary to light the LED.
typedef struct EsfLedManagerLedInfo {
  uint32_t color;                     // The color to be lit.
  EsfLedManagerLedInterval interval;  // Interval
} EsfLedManagerLedInfo;

// This structure is used for the state priority table.
typedef struct EsfLedManagerPriority {
  EsfLedManagerLedStatus state;  // A structure containing state information.
  EsfLedManagerLedInfo led;      // It is a structure with lighting contents.
} EsfLedManagerPriority;

// A structure with LED priority table and size.
typedef struct EsfLedManagerLedTables {
  const EsfLedManagerPriority* table;  // LED priority table.
  size_t size;                         // table size.
} EsfLedManagerLedTables;

#endif  // ESF_LED_MANAGER_CONFIG_TABLE_STRUCT_H_
