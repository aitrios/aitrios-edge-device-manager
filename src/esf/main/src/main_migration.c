/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "main_migration.h"

#include <stdbool.h>

#include "main.h"
#include "main_log.h"
#include "network_manager.h"
#include "power_manager.h"
#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "pl_main.h"

// ----------------------------------------------------------------------------
typedef struct EsfMigrationMask {
  uint8_t migration_done : 1;
} EsfMigrationMask;

static bool MigrationMaskIsEnabled(EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(EsfMigrationMask,
                                                       migration_done, mask);
}
typedef struct {
  uint32_t size;
  int32_t migration_done;
} EsfMigrationData;

static const EsfParameterStorageManagerMemberInfo kMigrationMembersInfo[] = {{
    .id = kEsfParameterStorageManagerItemMigrationDone,
    .type = kEsfParameterStorageManagerItemTypeRaw,
    .offset = 0,
    .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(EsfMigrationData,
                                                     migration_done),
    .enabled = MigrationMaskIsEnabled,
    .custom = NULL,
}};

static const EsfParameterStorageManagerStructInfo kEsfMigrationStructInfo = {
    .items_num = sizeof(kMigrationMembersInfo) / sizeof(kMigrationMembersInfo[0]),
    .items = kMigrationMembersInfo,
};

// ----------------------------------------------------------------------------
EsfMainError EsfMainIsNeedMigration(bool *ret) {
  if (!PlMainIsMigrationSupported()) {
    *ret = false;
    return kEsfMainOk;
  }

  EsfMainError esf_ret = kEsfMainOk;
  EsfParameterStorageManagerHandle handle;
  EsfParameterStorageManagerStatus storage_ret =
      kEsfParameterStorageManagerStatusOk;

  storage_ret = EsfParameterStorageManagerOpen(&handle);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    ESF_MAIN_ERR("PSM Open error:%u", storage_ret);
    return kEsfMainErrorExternal;
  }

  const int kInvalidVal = 999;
  EsfMigrationMask mask = {1};
  EsfMigrationData data = {.migration_done = kInvalidVal};
  storage_ret = EsfParameterStorageManagerLoad(
      handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)&data, &kEsfMigrationStructInfo, NULL);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    esf_ret = kEsfMainErrorExternal;
    goto end;
  }
  *ret = data.migration_done == 1 ? false : true;

end:
  storage_ret = EsfParameterStorageManagerClose(handle);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    ESF_MAIN_ERR("PSM Close error:%u", storage_ret);
  }
  return esf_ret;
}
// ----------------------------------------------------------------------------
EsfMainError EsfMainDisableMigrationFlag(void) {
  EsfMainError ret = kEsfMainOk;
  EsfParameterStorageManagerHandle handle;
  EsfParameterStorageManagerStatus storage_ret =
      kEsfParameterStorageManagerStatusOk;

  storage_ret = EsfParameterStorageManagerOpen(&handle);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    ESF_MAIN_ERR("PSM Open error:%u", storage_ret);
    return kEsfMainErrorExternal;
  }

  EsfMigrationMask mask = {1};
  EsfMigrationData data = {.migration_done = 1};
  storage_ret = EsfParameterStorageManagerSave(
      handle, (EsfParameterStorageManagerMask)&mask,
      (EsfParameterStorageManagerData)&data, &kEsfMigrationStructInfo, NULL);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    ESF_MAIN_LOG_ERR("PSM Save error:%u", storage_ret);
    ret = kEsfMainErrorExternal;
  }

  storage_ret = EsfParameterStorageManagerClose(handle);
  if (storage_ret != kEsfParameterStorageManagerStatusOk) {
    ESF_MAIN_ERR("PSM Close error:%u", storage_ret);
  }
  return ret;
}
// ----------------------------------------------------------------------------
EsfMainError EsfMainExecMigration(void) {
  ESF_MAIN_LOG_INFO("%s start", __func__);
  EsfMainError ret = kEsfMainOk;
  EsfPwrMgrError pwr_ret = EsfPwrMgrExecMigration();
  if (pwr_ret != kEsfPwrMgrOk) {
    ESF_MAIN_LOG_ERR("%s error:%u", __func__, pwr_ret);
    ret = kEsfMainErrorExternal;
  }
  EsfNetworkManagerResult nm_ret = EsfNetworkManagerExecMigration();
  if (nm_ret != kEsfNetworkManagerResultSuccess) {
    ESF_MAIN_LOG_ERR("%s error:%u", __func__, nm_ret);
    ret = kEsfMainErrorExternal;
  }
  ESF_MAIN_LOG_INFO("%s end:%u", __func__, ret);
  return ret;
}
