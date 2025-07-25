/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_pl.h"

#include "parameter_storage_manager/src/parameter_storage_manager_buffer.h"
#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"
#include ESF_PARAMETER_STORAGE_MANAGER_PL_STORAGE_FILE

#if defined(CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_PL_STORAGE_STUB)
#define PlStorageOpen PSM_PlStorageOpen
#define PlStorageClose PSM_PlStorageClose
#define PlStorageSeek PSM_PlStorageSeek
#define PlStorageRead PSM_PlStorageRead
#define PlStorageWrite PSM_PlStorageWrite
#define PlStorageErase PSM_PlStorageErase
#define PlStorageDRead PSM_PlStorageDRead
#define PlStorageDWrite PSM_PlStorageDWrite
#define PlStorageGetDataInfo PSM_PlStorageGetDataInfo
#define PlStorageSwitchData PSM_PlStorageSwitchData
#define PlStorageGetTmpDataId PSM_PlStorageGetTmpDataId
#define PlStorageInitialize PSM_PlStorageInitialize
#define PlStorageFinalize PSM_PlStorageFinalize
#define PlStorageGetCapabilities PSM_PlStorageGetCapabilities
#define PlStorageGetIdCapabilities PSM_PlStorageGetIdCapabilities
#define PlStorageFactoryReset PSM_PlStorageFactoryReset
#define PlStorageClean PSM_PlStorageClean
#define PlStorageDowngrade PSM_PlStorageDowngrade
#endif

static const int kPlStorageCopySize = UINT16_MAX;  // 64KB

// """Copies data from the actual data storage area to the temporary data
// storage area.

// Copies data from the actual data storage area to the temporary data storage
// area.

// Args:
//     [IN] src_id (EsfParameterStorageManagerItemID): Source ID.
//     [IN] dst_id (PlStorageTmpDataId): Destination ID.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): working
//     information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed. kEsfParameterStorageManagerStatusDataLoss: Unable to
//     access data storage area. kEsfParameterStorageManagerStatusUnavailable:
//     Failed to copy data.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterCopyPLStorage(
    EsfParameterStorageManagerItemID src_id, PlStorageTmpDataId dst_id,
    EsfParameterStorageManagerWorkMemberData* member);

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterConvertPlResult(PlErrCode status) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (status < 0 || kPlErrCodeMax <= status) {
      ret = kEsfParameterStorageManagerStatusInternal;
      break;
    }
    if (status == kPlErrCodeOk) {
      ret = kEsfParameterStorageManagerStatusOk;
      break;
    }
    if (status == kPlErrCodeError) {
      ret = kEsfParameterStorageManagerStatusDataLoss;
      break;
    }
    // ReadOnly File create, ReadOnly File Erase, No tmp id
    if (status == kPlErrNotFound || status == kPlErrInvalidOperation ||
        status == kPlErrInvalidParam) {
      ret = kEsfParameterStorageManagerStatusPermissionDenied;
      break;
    }
    ret = kEsfParameterStorageManagerStatusUnavailable;
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSavePLStorage(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const void* buf, uint32_t* outsize,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageHandle handle = NULL;
  PlStorageDataId data_id = PlStorageDataMax;

  do {
    if (member == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "NULL member. id=%u, ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (member->update) {
      data_id = (PlStorageDataId)member->update_data;
    } else {
      data_id =
          EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(
              id);
      if (data_id == PlStorageDataMax) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid ItemID. id=%u, ret=%u(%s)", id, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
      }
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Saving to PLStorage. id=%u, data_id=%u, offset=%" PRIu32
        ", size=%" PRIu32,
        id, data_id, offset, size);

    int32_t pos = 0;
    int oflags = member->append ? PL_STORAGE_OPEN_RDWR : PL_STORAGE_OPEN_WRONLY;
    PlErrCode pl_ret = PlStorageOpen(data_id, oflags, &handle);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Open failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id, data_id,
          pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    pl_ret = PlStorageSeek(handle, offset, PlStorageSeekSet, &pos);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Seek failed. id=%u, data_id=%u, offset=%" PRIu32
          ", pl_ret=%u, ret=%u(%s)",
          id, data_id, offset, pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    pl_ret = PlStorageWrite(handle, buf, size, outsize);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Write failed. id=%u, data_id=%u, size=%" PRIu32
          ", pl_ret=%u, ret=%u(%s)",
          id, data_id, size, pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Save successful. id=%u, data_id=%u, size_written=%" PRIu32, id,
        data_id, *outsize);
  } while (0);

  if (handle != NULL) {
    PlErrCode pl_ret = PlStorageClose(handle);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Close failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id, data_id,
          pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadPLStorage(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, void* buf, uint32_t* outsize) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageHandle handle = NULL;
  PlStorageDataId data_id = PlStorageDataMax;

  do {
    if (member != NULL && member->update) {
      data_id = (PlStorageDataId)member->update_data;
    } else {
      data_id =
          EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(
              id);
      if (data_id == PlStorageDataMax) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
      }
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Loading from PLStorage. id=%u, data_id=%u, offset=%" PRIu32
        ", size=%" PRIu32,
        id, data_id, offset, size);

    int32_t pos = 0;
    PlErrCode pl_ret = PlStorageOpen(data_id, PL_STORAGE_OPEN_RDONLY, &handle);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Open failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id, data_id,
          pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    pl_ret = PlStorageSeek(handle, offset, PlStorageSeekSet, &pos);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Seek failed. id=%u, data_id=%u, offset=%" PRIu32
          ", pl_ret=%u, ret=%u(%s)",
          id, data_id, offset, pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    pl_ret = PlStorageRead(handle, buf, size, outsize);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Read failed. id=%u, data_id=%u, size=%" PRIu32
          ", pl_ret=%u, ret=%u(%s)",
          id, data_id, size, pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Load successful. id=%u, data_id=%u, size_read=%" PRIu32, id, data_id,
        *outsize);
  } while (0);

  if (handle != NULL) {
    PlErrCode pl_ret = PlStorageClose(handle);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Close failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id, data_id,
          pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterClearPLStorage(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageDataId data_id = PlStorageDataMax;

  do {
    if (member != NULL && member->update) {
      data_id = (PlStorageDataId)member->update_data;
    } else {
      data_id =
          EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(
              id);
      if (data_id == PlStorageDataMax) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
      }
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clearing PLStorage. id=%u, data_id=%u",
                                        id, data_id);

    PlErrCode pl_ret = PlStorageErase(data_id);
    if (pl_ret != kPlErrCodeOk && pl_ret != kPlErrNotFound) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Erase failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id, data_id,
          pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    if (pl_ret == kPlErrNotFound) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Storage already empty. id=%u, data_id=%u", id, data_id);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Clear successful. id=%u, data_id=%u",
                                          id, data_id);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterFactoryResetPLStorage(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageDataId data_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(id);

  do {
    if (data_id == PlStorageDataMax) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Factory resetting PLStorage. id=%u, data_id=%u", id, data_id);

    // In case of PL Storage, delete is performed instead of Factory Reset.
    PlErrCode pl_ret = PlStorageFactoryReset(data_id);
    if (pl_ret != kPlErrCodeOk && pl_ret != kPlErrNotFound) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Factory reset failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id,
          data_id, pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    if (pl_ret == kPlErrNotFound) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Storage already empty. id=%u, data_id=%u", id, data_id);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Factory reset successful. id=%u, data_id=%u", id, data_id);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetStorageInfoPLStorage(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member,
    EsfParameterStorageManagerStorageInfo* storage) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageDataId data_id = PlStorageDataMax;

  do {
    if (member != NULL && member->update) {
      data_id = (PlStorageDataId)member->update_data;
    } else {
      data_id =
          EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(
              id);
      if (data_id == PlStorageDataMax) {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
      }
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Getting storage info for PLStorage. id=%u, data_id=%u", id, data_id);

    PlStorageDataInfo info;
    PlErrCode pl_ret = PlStorageGetDataInfo(data_id, &info);
    if (pl_ret != kPlErrCodeOk && pl_ret != kPlErrNotFound) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Get data info failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)", id,
          data_id, pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }
    if (pl_ret == kPlErrNotFound) {
      storage->written_size = 0;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Storage is empty. id=%u, data_id=%u",
                                          id, data_id);
    } else {
      storage->written_size = info.written_size;
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Storage info retrieved. id=%u, data_id=%u, written_size=%" PRIu32,
          id, data_id, storage->written_size);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateBeginPLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerUpdateType type,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageDataId data_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(id);
  do {
    PlStorageTmpDataId tmp_id = 0;
    PlErrCode pl_ret = kPlErrCodeOk;

    if (data_id == PlStorageDataMax) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Starting update for PLStorage. id=%u, data_id=%u, type=%u", id,
        data_id, type);

    pl_ret = PlStorageGetTmpDataId(data_id, &tmp_id);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Get temp data id failed. id=%u, data_id=%u, pl_ret=%u, ret=%u(%s)",
          id, data_id, pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    switch (type) {
      case kEsfParameterStorageManagerUpdateEmpty: {
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Temp data erased. id=%u, tmp_id=%u", id, tmp_id);
        break;
      }
      case kEsfParameterStorageManagerUpdateCopy: {
        if (member->storage.written_size == 0) {
          ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
              "Temp data erased (empty source). id=%u, tmp_id=%u", id, tmp_id);
          break;
        }
        ret = EsfParameterStorageManagerStorageAdapterCopyPLStorage(id, tmp_id,
                                                                    member);
        if (ret != kEsfParameterStorageManagerStatusOk) {
          ESF_PARAMETER_STORAGE_MANAGER_ERROR(
              "Copy to temp area failed. id=%u, data_id=%u, tmp_id=%u, "
              "ret=%u(%s)",
              id, data_id, tmp_id, ret,
              EsfParameterStorageManagerStrError(ret));
          break;
        }
        ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
            "Data copied to temp area. id=%u, data_id=%u, tmp_id=%u", id,
            data_id, tmp_id);
        break;
      }
      case kEsfParameterStorageManagerUpdateTypeMax:
      default: {
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Invalid update type. id=%u, type=%u, ret=%u(%s)", id, type, ret,
            EsfParameterStorageManagerStrError(ret));
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
      }
    }
    if (ret == kEsfParameterStorageManagerStatusOk) {
      member->update = true;
      member->update_data = (uintptr_t)tmp_id;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Update begin successful. id=%u, tmp_id=%u", id, tmp_id);
    }
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCompletePLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageDataId data_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(id);
  do {
    if (data_id == PlStorageDataMax) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    PlStorageTmpDataId tmp_id = (PlStorageTmpDataId)member->update_data;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Completing update for PLStorage. id=%u, data_id=%u, tmp_id=%u", id,
        data_id, tmp_id);

    PlErrCode pl_ret = PlStorageSwitchData(tmp_id, data_id);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Switch temp data failed. id=%u, data_id=%u, tmp_id=%u, pl_ret=%u, "
          "ret=%u(%s)",
          id, data_id, tmp_id, pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Update complete successful. id=%u, data_id=%u, tmp_id=%u", id, data_id,
        tmp_id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCancelPLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  PlStorageDataId data_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(id);
  do {
    if (data_id == PlStorageDataMax) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    PlStorageTmpDataId tmp_id = (PlStorageTmpDataId)member->update_data;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Cancelling update for PLStorage. id=%u, data_id=%u, tmp_id=%u", id,
        data_id, tmp_id);

    PlErrCode pl_ret = PlStorageErase(tmp_id);
    if (pl_ret != kPlErrCodeOk && pl_ret != kPlErrNotFound) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Erase temp data failed. id=%u, data_id=%u, tmp_id=%u, pl_ret=%u, "
          "ret=%u(%s)",
          id, data_id, tmp_id, pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    if (pl_ret == kPlErrNotFound) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Temp data already erased. id=%u, data_id=%u, tmp_id=%u", id, data_id,
          tmp_id);
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Update cancel successful. id=%u, data_id=%u, tmp_id=%u", id, data_id,
          tmp_id);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterInitPLStorage(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Initializing PL Storage");

  PlErrCode pl_ret = PlStorageInitialize();
  if (pl_ret != kPlErrCodeOk) {
    // Since the policy is to ignore any errors that occur in PL Storage, set
    // the log level to Warning.
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "PL Storage initialization failed. pl_ret=%u, ret=%u(%s)", pl_ret, ret,
        EsfParameterStorageManagerStrError(ret));
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);

  } else {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("PL Storage initialized successfully");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDeinitPLStorage(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Deinitializing PL Storage");

  PlErrCode pl_ret = PlStorageFinalize();
  if (pl_ret != kPlErrCodeOk) {
    // Since the policy is to ignore any errors that occur in PL Storage, set
    // the log level to Warning.
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "PL Storage deinitialization failed. pl_ret=%u, ret=%u(%s)", pl_ret,
        ret, EsfParameterStorageManagerStrError(ret));
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "PL Storage deinitialized successfully");
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterCopyPLStorage(
    EsfParameterStorageManagerItemID src_id, PlStorageTmpDataId dst_id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  bool is_allocated = false;
  EsfParameterStorageManagerBuffer buffer =
      ESF_PARAMETER_STORAGE_MANAGER_BUFFER_INITIALIZER;

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
      "Copying PL Storage. src_id=%u, dst_id=%u", src_id, dst_id);

  do {
    if (kPlStorageCopySize < member->storage.written_size) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "The target data is too large. size=%" PRIu32 ", ret=%u(%s)",
          member->storage.written_size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INTERNAL_PROCESS_ERROR);
      break;
    }
    ret = EsfParameterStorageManagerBufferAllocate(member->storage.written_size,
                                                   &buffer);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Copy buffer allocation failed. size=%d, ret=%u(%s)",
          kPlStorageCopySize, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }
    is_allocated = true;
    ret = EsfParameterStorageManagerBufferLoad(&buffer, src_id, member);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load to copy buffer failed. src_id=%u, dst_id=%u, ret=%u(%s)",
          src_id, dst_id, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }
    member->update = true;
    member->update_data = (uintptr_t)dst_id;
    ret = EsfParameterStorageManagerBufferSave(&buffer, src_id, member);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save the copy buffer failed. src_id=%u, dst_id=%u, ret=%u(%s)",
          src_id, dst_id, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Copy completed. src_id=%d, dst_id=%d",
                                        src_id, dst_id);
  } while (0);

  if (is_allocated) {
    EsfParameterStorageManagerStatus buf_ret =
        EsfParameterStorageManagerBufferFree(&buffer);
    if (buf_ret != kEsfParameterStorageManagerStatusOk) {
      ret = buf_ret;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Releasing the copy buffer failed. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
    }
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetCapabilitiesPLStorage(
    EsfParameterStorageManagerCapabilities* capabilities) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("Getting PL Storage capabilities");

  do {
    if (capabilities == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument. capabilities=NULL, ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    PlStorageCapabilities pl_capability;

    PlErrCode pl_ret = PlStorageGetCapabilities(&pl_capability);
    if (pl_ret != kPlErrCodeOk) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Get capabilities failed. pl_ret=%u, ret=%u(%s)", pl_ret, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    capabilities->cancellable = pl_capability.enable_tmp_id;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "PL Storage capabilities retrieved. cancellable=%d",
        capabilities->cancellable);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetItemCapabilitiesPLStorage(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerItemCapabilities* capabilities) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Getting PL Storage item capabilities. id=%u", id);

  do {
    if (capabilities == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument. capabilities=NULL, id=%u, ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    PlStorageDataId data_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToPLStorageID(id);
    if (data_id == PlStorageDataMax) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid ItemID. id=%u, data_id=%u, ret=%u(%s)", id, data_id, ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    PlStorageIdCapabilities pl_capability;
    PlErrCode pl_ret = PlStorageGetIdCapabilities(data_id, &pl_capability);
    if (pl_ret != kPlErrCodeOk) {
      ret = EsfParameterStorageManagerStorageAdapterConvertPlResult(pl_ret);
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Get item capabilities failed. id=%u, data_id=%u, pl_ret=%u, "
          "ret=%u(%s)",
          id, data_id, pl_ret, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
      break;
    }

    capabilities->read_only = pl_capability.is_read_only;
    capabilities->enable_offset = pl_capability.enable_seek;
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Item capabilities retrieved. id=%u, data_id=%u, read_only=%d, "
        "enable_offset=%d",
        id, data_id, capabilities->read_only, capabilities->enable_offset);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterCleanPLStorage(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  PlErrCode pl_ret = PlStorageClean();
  if (pl_ret != kPlErrCodeOk) {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "PlStorageClean failed. pl_ret=%u, ret=%u(%s)", pl_ret, ret,
        EsfParameterStorageManagerStrError(ret));
    ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_PORTING_LAYER_FAILURE);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDowngradePLStorage(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  PlErrCode pl_ret = PlStorageDowngrade();
  if (pl_ret != kPlErrCodeOk) {
    ret = kEsfParameterStorageManagerStatusInternal;
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "PlStorageDowngrade failed. pl_ret=%u, ret=%u(%s)", pl_ret, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}
