/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_LED_HW_H__
#define __PL_LED_HW_H__

// Typedef ---------------------------------------------------------------------
typedef struct led_hw_info {
  uint32_t        leds_num;
  PlLedLedsInfo   leds[CONFIG_PL_LED_LEDS_NUM];
} PlLedHwInfo;

// Private API------------------------------------------------------------------
PlErrCode PlLedHwInitialize(void);
PlErrCode PlLedHwFinalize(void);
PlErrCode PlLedHwOn(uint32_t led_id, uint32_t color_id);
PlErrCode PlLedHwOff(uint32_t led_id);
PlErrCode PlLedHwGetInfo(PlLedHwInfo *dev_info);

#endif /* __PL_LED_HW_H__ */
