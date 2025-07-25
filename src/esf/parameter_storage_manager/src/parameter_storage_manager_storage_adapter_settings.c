/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings_table.h"

EsfParameterStorageManagerStorageID
EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStorageID ret =
      kEsfParameterStorageManagerStorageMax;
  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, range=[0, %d]", id,
          kEsfParameterStorageManagerItemCustom - 1);
      break;
    }
    ret = kDataIdTable[id].storage;
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit id=%u", ret);
  return ret;
}

PlStorageDataId
EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  PlStorageDataId ret = PlStorageDataMax;
  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, range=[0, %d]", id,
          kEsfParameterStorageManagerItemCustom - 1);
      break;
    }
    ret = kDataIdTable[id].data_id.pl_storage;
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit id=%u", ret);
  return ret;
}

EsfParameterStorageManagerStorageOtherDataID
EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStorageOtherDataID ret =
      kEsfParameterStorageManagerStorageOtherDataMax;
  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, range=[0, %d]", id,
          kEsfParameterStorageManagerItemCustom - 1);
      break;
    }
    ret = kDataIdTable[id].data_id.other;
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit id=%u", ret);
  return ret;
}

bool EsfParameterStorageManagerStorageAdapterConvertItemIDToFactoryResetRequired(  // NOLINT
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  bool ret = false;
  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, range=[0, %d]", id,
          kEsfParameterStorageManagerItemCustom - 1);
      break;
    }
    ret = kDataIdTable[id].factory_reset_required;
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit ret=%s", ret ? "true" : "false");
  return ret;
}
