/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_LED_MANAGER_CONFIG_CONFIG_H_
#define ESF_LED_MANAGER_CONFIG_CONFIG_H_

#ifdef CONFIG_EXTERNAL_INTEGRATION_TEST_LED_MANAGER
#define LED_MANAGER_STATIC
#define LED_MANAGER_CONSTANT
#else
#define LED_MANAGER_STATIC static
#define LED_MANAGER_CONSTANT const
#endif

#include "led_manager/config/table_struct.h"
#include "led_manager/config/led_table.h"

#endif  // ESF_LED_MANAGER_CONFIG_CONFIG_H_
