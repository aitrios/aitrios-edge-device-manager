/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pl_system_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#define PL_SYSTEM_MANAGER_HWINFO_MAX_SIZE (64)
#define PL_SYSTEM_MANAGER_DEFAULT_DEVICE_ID_DIR "/etc/evp"

static PlErrCode PlSystemManagerGetSerialNumber(char *serial_number);

size_t PlSystemManagerGetHwInfoSize(void) {
  return (size_t)PL_SYSTEM_MANAGER_HWINFO_MAX_SIZE;
}

PlErrCode PlSystemManagerParseHwInfo(char *hw_info,
                                     PlSystemManagerHwInfo *data) {
  (void)hw_info;  // rpi porting layer does not use this parameter

  if ((data == NULL)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Parameter error. data=%p.",
                     "pl_system_manager_impl.c", __LINE__, data);
    return kPlErrInvalidParam;
  }

  data->model_name[0] = '\0';
  data->manufacturer_name[0] = '\0';
  data->product_serial_number[0] = '\0';
  data->serial_number[0] = '\0';
  data->aiisp_chip_id[0] = '\0';
  data->sensor_id[0] = '\0';
  data->app_processor_type[0] = '\0';
  data->sensor_model_name[0] = '\0';

  PlErrCode err = PlSystemManagerGetSerialNumber(data->serial_number);
  if (err != kPlErrCodeOk) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to get serial number. err=%d.",
                     "pl_system_manager_impl.c", __LINE__, err);
    return err;
  }

  return kPlErrCodeOk;
}

static PlErrCode PlSystemManagerGetSerialNumber(char *serial_number) {
  const char *dir_path = getenv("DEVICE_ID_DIR_PATH");
  if (!dir_path) {
    dir_path = PL_SYSTEM_MANAGER_DEFAULT_DEVICE_ID_DIR;
  }

  DIR *dir = opendir(dir_path);
  if (dir == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:Failed to open dir. dir_path=%s.",
                     "pl_system_manager_impl.c", __LINE__, dir_path);
    return kPlErrNotFound;
  }

  struct dirent *dp;
  dp = readdir(dir);
  while (dp != NULL) {
    if (strstr(dp->d_name, "_cert.pem") != NULL) {
      break;
    }
    dp = readdir(dir);
  }
  closedir(dir);

  if (dp == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:No _cert.pem file found in dir_path=%s.",
                     "pl_system_manager_impl.c", __LINE__, dir_path);
    return kPlErrNotFound;
  }

  char *device_id_name = NULL;
  char *saveptr = NULL;
  device_id_name = strtok_r(dp->d_name, "_", &saveptr);
  if (device_id_name == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:Failed to parse device id from file name: %s.",
                     "pl_system_manager_impl.c", __LINE__, dp->d_name);
    return kPlErrInvalidValue;
  }

  if (strcmp(device_id_name, "cert.pem") == 0) {
    device_id_name[0] = '\0';
  }

  memcpy(serial_number, device_id_name,
         PL_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE);
  serial_number[PL_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE - 1] = '\0';

  return kPlErrCodeOk;
}

bool PlSystemManagerIsHwInfoSupported(void) { return false; }
