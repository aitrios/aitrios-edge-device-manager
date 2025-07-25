/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_H_

#include "firmware_manager_esp32_impl.h"
#include "firmware_manager_porting_layer.h"

PlErrCode FwMgrEsp32FwGetOps(FwMgrEsp32ImplOps *ops);

PlErrCode FwMgrEsp32FwGetHashAndUpdateDate(int32_t hash_size, uint8_t *hash,
                                           int32_t update_date_size,
                                           char *update_date);
void FwMgrEsp32FwMigrateFromV1(void);

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_FIRMWARE_MANAGER_ESP32_FW_H_
