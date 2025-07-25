/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_other.h"

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveOther(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const void* buf, uint32_t* outsize,
    EsfParameterStorageManagerWorkMemberData* member) {
  (void)offset;
  (void)size;
  (void)buf;
  (void)outsize;
  (void)member;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerStorageOtherDataID data_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(id);
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Converting ItemID to OtherDataID. id=%u, data_id=%u", id, data_id);

    switch (data_id) {
      case kEsfParameterStorageManagerStorageOtherDataMax:
      default:
        ret = kEsfParameterStorageManagerStatusPermissionDenied;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown OtherDataID. id=%u, data_id=%u, ret=%u(%s)", id, data_id,
            ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadOther(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, void* buf, uint32_t* outsize) {
  (void)member;
  (void)offset;
  (void)size;
  (void)buf;
  (void)outsize;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerStorageOtherDataID data_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(id);
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Converting ItemID to OtherDataID. id=%u, data_id=%u", id, data_id);

    switch (data_id) {
      case kEsfParameterStorageManagerStorageOtherDataMax:
      default:
        ret = kEsfParameterStorageManagerStatusPermissionDenied;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown OtherDataID. id=%u, data_id=%u, ret=%u(%s)", id, data_id,
            ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterClearOther(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member) {
  (void)member;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerStorageOtherDataID data_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(id);
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Converting ItemID to OtherDataID. id=%u, data_id=%u", id, data_id);

    switch (data_id) {
      case kEsfParameterStorageManagerStorageOtherDataMax:
      default:
        ret = kEsfParameterStorageManagerStatusPermissionDenied;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown OtherDataID. id=%u, data_id=%u, ret=%u(%s)", id, data_id,
            ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterFactoryResetOther(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerStorageOtherDataID data_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(id);
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Converting ItemID to OtherDataID for factory reset. id=%u, data_id=%u",
        id, data_id);

    switch (data_id) {
      case kEsfParameterStorageManagerStorageOtherDataMax:
      default:
        ret = kEsfParameterStorageManagerStatusPermissionDenied;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown OtherDataID for factory reset. id=%u, data_id=%u, "
            "ret=%u(%s)",
            id, data_id, ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetStorageInfoOther(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member,
    EsfParameterStorageManagerStorageInfo* storage) {
  (void)member;
  (void)storage;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    EsfParameterStorageManagerStorageOtherDataID data_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToOtherDataID(id);
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Converting ItemID to OtherDataID for storage info. id=%u, data_id=%u",
        id, data_id);

    switch (data_id) {
      case kEsfParameterStorageManagerStorageOtherDataMax:
      default:
        ret = kEsfParameterStorageManagerStatusPermissionDenied;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown OtherDataID for storage info. id=%u, data_id=%u, "
            "ret=%u(%s)",
            id, data_id, ret, EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateBeginOther(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerUpdateType type,
    EsfParameterStorageManagerWorkMemberData* member) {
  (void)id;
  (void)type;
  (void)member;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      kEsfParameterStorageManagerStatusInternal;

  ESF_PARAMETER_STORAGE_MANAGER_ERROR(
      "Update cancellation not supported for other data storage. id=%u, "
      "type=%u, ret=%u(%s)",
      id, type, ret, EsfParameterStorageManagerStrError(ret));
  ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCompleteOther(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  (void)id;
  (void)member;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      kEsfParameterStorageManagerStatusInternal;

  ESF_PARAMETER_STORAGE_MANAGER_ERROR(
      "Update completion not supported for other data storage. id=%u, "
      "ret=%u(%s)",
      id, ret, EsfParameterStorageManagerStrError(ret));
  ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCancelOther(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  (void)id;
  (void)member;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      kEsfParameterStorageManagerStatusInternal;

  ESF_PARAMETER_STORAGE_MANAGER_ERROR(
      "Update cancellation not supported for other data storage. id=%u, "
      "ret=%u(%s)",
      id, ret, EsfParameterStorageManagerStrError(ret));
  ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterInitOther(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDeinitOther(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterCleanOther(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDowngradeOther(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}
