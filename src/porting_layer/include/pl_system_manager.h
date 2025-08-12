/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// This header file defines structures and functions related to the HW info
// data management in the parameter storage manager.

#ifndef PL_SYSTEM_MANAGER_H_
#define PL_SYSTEM_MANAGER_H_

#include <stdbool.h>
#include <stddef.h>

#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PL_SYSTEM_MANAGER_HWINFO_MODEL_NAME_MAX_SIZE (33)
#define PL_SYSTEM_MANAGER_HWINFO_MANUFACTURER_NAME_MAX_SIZE (33)
#define PL_SYSTEM_MANAGER_HWINFO_PRODUCT_SERIAL_NUMBER_MAX_SIZE (33)
#define PL_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE (64)
#define PL_SYSTEM_MANAGER_HWINFO_AIISP_CHIP_ID_MAX_SIZE (37)
#define PL_SYSTEM_MANAGER_HWINFO_SENSOR_ID_MAX_SIZE (37)
#define PL_SYSTEM_MANAGER_HWINFO_APP_PROCESSOR_TYPE_MAX_SIZE (64)
#define PL_SYSTEM_MANAGER_HWINFO_SENSOR_MODEL_NAME_MAX_SIZE (64)

typedef struct PlSystemManagerHwInfo {
  char model_name[PL_SYSTEM_MANAGER_HWINFO_MODEL_NAME_MAX_SIZE];  // Model Name.
  char manufacturer_name
      [PL_SYSTEM_MANAGER_HWINFO_MANUFACTURER_NAME_MAX_SIZE];  // Manufacturer
                                                              // Name.
  char product_serial_number
      [PL_SYSTEM_MANAGER_HWINFO_PRODUCT_SERIAL_NUMBER_MAX_SIZE];  // Product
                                                                  // Serial
                                                                  // Number.
  char serial_number
      [PL_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE];  // Serial Number.
  char aiisp_chip_id
      [PL_SYSTEM_MANAGER_HWINFO_AIISP_CHIP_ID_MAX_SIZE];  // AIISP Chip ID.
  char sensor_id[PL_SYSTEM_MANAGER_HWINFO_SENSOR_ID_MAX_SIZE];  // Sensor ID.
  char app_processor_type
      [PL_SYSTEM_MANAGER_HWINFO_APP_PROCESSOR_TYPE_MAX_SIZE];  // Application
                                                               // Processor
                                                               // Type.
  char sensor_model_name
      [PL_SYSTEM_MANAGER_HWINFO_SENSOR_MODEL_NAME_MAX_SIZE];  // Sensor Model
                                                              // Name.
} PlSystemManagerHwInfo;

typedef enum {
  kPlSystemManagerResetCauseUnknown = -1,
  kPlSystemManagerResetCauseSysChipPowerOnReset = 0,
  kPlSystemManagerResetCauseSysBrownOut,
  kPlSystemManagerResetCauseCoreSoft,
  kPlSystemManagerResetCauseCoreDeepSleep,
  kPlSystemManagerResetCauseWDT,
  kPlSystemManagerResetCauseDefault,
  kPlSystemManagerResetCauseClear,
  kPlSystemManagerResetCauseMax
} PlSystemManagerResetCause;

size_t PlSystemManagerGetHwInfoSize(void);

PlErrCode PlSystemManagerParseHwInfo(char *hw_info,
                                     PlSystemManagerHwInfo *data);

bool PlSystemManagerIsHwInfoSupported(void);

PlErrCode PlSystemManagerIsNeedReboot(PlSystemManagerResetCause reset_cause,
                                      bool *reset_flag);

#ifdef __cplusplus
}
#endif

#endif  // PL_SYSTEM_MANAGER_H_
