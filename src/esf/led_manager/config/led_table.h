/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_LED_MANAGER_CONFIG_LED_TABLE_H_
#define ESF_LED_MANAGER_CONFIG_LED_TABLE_H_

#ifdef CONFIG_EXTERNAL_LED_MANAGER_DISABLE
#include "led_manager/config/stub.inc"
#else
#ifdef CONFIG_EXTERNAL_LED_MANAGER_T3S3_AITRIOS
#include "led_manager/config/t3s3_aitrios.inc"
#endif
#endif  /* CONFIG_EXTERNAL_LED_MANAGER_DISABLE */

#endif  // ESF_LED_MANAGER_CONFIG_LED_TABLE_H_
