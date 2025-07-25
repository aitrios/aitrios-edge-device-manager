/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter.h"

#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_other.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_pl.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"
#include "parameter_storage_manager/src/parameter_storage_manager_utility.h"

// """Saves data to the data storage area.

// The Save API is called based on the ItemID.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be saved.
//     [IN] data (const uint8_t*): Data to be saved.
//     [OUT] save_size (uint32_t*): Data size to be saved.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): working
//     information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument info is an
//     invalid value. kEsfParameterStorageManagerStatusInternal: Parameter
//     Storage Manager was not initialized.
//     kEsfParameterStorageManagerStatusInternal: Internal processing failed.
//     kEsfParameterStorageManagerStatusDataLoss: Unable to access data storage
//     area. kEsfParameterStorageManagerStatusUnavailable: Failed to save data.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveInternal(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const uint8_t* data, uint32_t* save_size,
    EsfParameterStorageManagerWorkMemberData* member);

// """Checks whether it is possible to call the Save API.

// Checks whether it is possible to call the Save API.
// If the call is not succeesful, an error code is returned.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be saved.
//     [IN] data (const uint8_t*): Data to be saved.
//     [OUT] save_size (uint32_t*): Data size to be saved.
//     [IN/OUT] member (EsfParameterStorageManagerWorkMemberData*): working
//     information.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//     invalid. kEsfParameterStorageManagerStatusOutOfRange: Invalid range was
//     specified. kEsfParameterStorageManagerStatusInternal: The argument info
//     is an invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterValidateSave(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const uint8_t* data, uint32_t* save_size,
    EsfParameterStorageManagerWorkMemberData* member);

// """Load data from the data storage area.

// The Load API is called based on the ItemID.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information. [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be saved.
//     [OUT] data (uint8_t*): Data to be saved.
//     [OUT] load_size (uint32_t*): Data size to be saved.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInternal: The argument is an invalid
//     value. kEsfParameterStorageManagerStatusInternal: Internal processing
//     failed. kEsfParameterStorageManagerStatusDataLoss: Unable to access data
//     storage area. kEsfParameterStorageManagerStatusUnavailable: Failed to
//     load data.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadInternal(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, uint8_t* data, uint32_t* load_size);

// """Checks whether it is possible to call the Load API.

// Checks whether it is possible to call the Load API.
// If the call is not succeesful, an error code is returned.

// Args:
//     [IN] id (EsfParameterStorageManagerItemID): Data type.
//     [IN] member (const EsfParameterStorageManagerWorkMemberData*): working
//     information. [IN] offset (uint32_t): The location where data is stored.
//     [IN] size (uint32_t): Data size to be loaded.
//     [IN] data (uint8_t*): Data to be loaded.
//     [OUT] load_size (uint32_t*): Data size to be loaded.

// Returns:
//     EsfParameterStorageManagerStatus: The code returns one of the values
//     EsfParameterStorageManagerStatus depending on the execution result.

// Yields:
//     kEsfParameterStorageManagerStatusOk: Success.
//     kEsfParameterStorageManagerStatusInvalidArgument: The argument is
//     invalid. kEsfParameterStorageManagerStatusOutOfRange: Invalid range was
//     specified. kEsfParameterStorageManagerStatusInternal: The argument info
//     is an invalid value. kEsfParameterStorageManagerStatusInternal: Internal
//     processing failed.

// Note:
// """
static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterValidateLoad(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, uint8_t* data, uint32_t* load_size);

EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterSave(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const uint8_t* data, uint32_t* save_size,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");

  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    ret = EsfParameterStorageManagerStorageAdapterValidateSave(
        id, offset, size, data, save_size, member);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments for save operation. id=%u, offset=%" PRIu32
          ", size=%" PRIu32 ". ret=%u(%s)",
          id, offset, size, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Performing save operation: id=%u, offset=%" PRIu32 ", size=%" PRIu32,
        id, offset, size);
    ret = EsfParameterStorageManagerStorageAdapterSaveInternal(
        id, offset, size, data, save_size, member);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Save operation failed. id=%u, offset=%" PRIu32 ", size=%" PRIu32
          ". ret=%u(%s)",
          id, offset, size, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Save operation successful. id=%u, saved_size=%" PRIu32, id,
        *save_size);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterLoad(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, uint8_t* data, uint32_t* load_size) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    ret = EsfParameterStorageManagerStorageAdapterValidateLoad(
        id, member, offset, size, data, load_size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments for load operation. id=%u, offset=%" PRIu32
          ", size=%" PRIu32 ". ret=%u(%s)",
          id, offset, size, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }

    if (member->storage.written_size == 0) {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "No data to load. Written size is 0.");
      *load_size = 0;
      break;
    }

    if (member->storage.written_size < offset + size) {
      uint32_t adjusted_size = member->storage.written_size - offset;
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Requested size exceeds available data. Adjusting size from %" PRIu32
          " to %" PRIu32,
          size, adjusted_size);
      size = adjusted_size;
    }

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Performing load operation: id=%u, offset=%" PRIu32 ", size=%" PRIu32,
        id, offset, size);
    ret = EsfParameterStorageManagerStorageAdapterLoadInternal(
        id, member, offset, size, data, load_size);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Load operation failed. id=%u, offset=%" PRIu32 ", size=%" PRIu32
          ". ret=%u(%s)",
          id, offset, size, ret, EsfParameterStorageManagerStrError(ret));
    } else {
      ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
          "Load operation successful. id=%u, loaded_size=%" PRIu32, id,
          *load_size);
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterClear(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
      "Clearing storage for item ID: %u, Storage ID: %u", id, storage_id);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Clearing PL storage");
      ret = EsfParameterStorageManagerStorageAdapterClearPLStorage(id, member);
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Clearing Other storage");
      ret = EsfParameterStorageManagerStorageAdapterClearOther(id, member);
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unknown storage ID for item ID: %u. ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    member->storage.written_size = 0;
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Storage cleared successfully for item ID: %u", id);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to clear storage for item ID: %u. ret=%u(%s)", id, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterFactoryReset(
    EsfParameterStorageManagerItemID id) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
      "Performing factory reset for item ID: %u, Storage ID: %u", id,
      storage_id);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Factory resetting PL storage");
      ret = EsfParameterStorageManagerStorageAdapterFactoryResetPLStorage(id);
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Factory resetting Other storage");
      ret = EsfParameterStorageManagerStorageAdapterFactoryResetOther(id);
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Unknown storage ID for item ID: %u. ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Factory reset successful for item ID: %u", id);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Factory reset failed for item ID: %u. ret=%u(%s)", id, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetStorageInfo(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member,
    EsfParameterStorageManagerStorageInfo* storage) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (storage == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument: storage is NULL. id=%u", id);
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "exit %d(%s)", ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    EsfParameterStorageManagerStorageID storage_id =
        EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Getting storage info for item ID: %u, Storage ID: %u", id, storage_id);

    switch (storage_id) {
      case kEsfParameterStorageManagerStoragePl: {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Retrieving PL storage info");
        ret = EsfParameterStorageManagerStorageAdapterGetStorageInfoPLStorage(
            id, member, storage);
        break;
      }
      case kEsfParameterStorageManagerStorageOther: {
        ESF_PARAMETER_STORAGE_MANAGER_TRACE("Retrieving Other storage info");
        ret = EsfParameterStorageManagerStorageAdapterGetStorageInfoOther(
            id, member, storage);
        break;
      }
      case kEsfParameterStorageManagerStorageMax:
      default: {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Unknown storage ID for item ID: %u", id);
        ret = kEsfParameterStorageManagerStatusInternal;
        ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
            ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
        break;
      }
    }
  } while (0);

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Successfully retrieved storage info for item ID: %u", id);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to retrieve storage info for item ID: %u. ret=%u(%s)", id, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateBegin(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerUpdateType type,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
      "Starting update for item ID: %u, Storage ID: %u, Update Type: %u", id,
      storage_id, type);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Initiating update for PL Storage");
      ret = EsfParameterStorageManagerStorageAdapterUpdateBeginPLStorage(
          id, type, member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to start update of PL Storage. id=%u, type=%u, ret=%u(%s)",
            id, type, ret, EsfParameterStorageManagerStrError(ret));
      }
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Initiating update for Other Storage");
      ret = EsfParameterStorageManagerStorageAdapterUpdateBeginOther(id, type,
                                                                     member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to start update of Other Storage. id=%u, type=%u, "
            "ret=%u(%s)",
            id, type, ret, EsfParameterStorageManagerStrError(ret));
      }
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Unknown storage ID for item ID: %u",
                                          id);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully started update for item ID: %u, Update Type: %u", id,
        type);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateComplete(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
      "Completing update for item ID: %u, Storage ID: %u", id, storage_id);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Completing update for PL Storage");
      ret = EsfParameterStorageManagerStorageAdapterUpdateCompletePLStorage(
          id, member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to complete update of PL Storage. id=%u, ret=%u(%s)", id,
            ret, EsfParameterStorageManagerStrError(ret));
      }
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Completing update for Other Storage");
      ret = EsfParameterStorageManagerStorageAdapterUpdateCompleteOther(id,
                                                                        member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to complete update of Other Storage. id=%u, ret=%u(%s)", id,
            ret, EsfParameterStorageManagerStrError(ret));
      }
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Unknown storage ID for item ID: %u",
                                          id);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully completed update for item ID: %u", id);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterUpdateCancel(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
      "Cancelling update for item ID: %u, Storage ID: %u", id, storage_id);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Cancelling update for PL Storage");
      ret = EsfParameterStorageManagerStorageAdapterUpdateCancelPLStorage(
          id, member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to cancel update of PL Storage. id=%u, ret=%u(%s)", id, ret,
            EsfParameterStorageManagerStrError(ret));
      }
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE(
          "Cancelling update for Other Storage");
      ret =
          EsfParameterStorageManagerStorageAdapterUpdateCancelOther(id, member);
      if (ret != kEsfParameterStorageManagerStatusOk) {
        ESF_PARAMETER_STORAGE_MANAGER_ERROR(
            "Failed to cancel update of Other Storage. id=%u, ret=%u(%s)", id,
            ret, EsfParameterStorageManagerStrError(ret));
      }
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Unknown storage ID for item ID: %u",
                                          id);
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Successfully cancelled update for item ID: %u", id);
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetCapabilities(
    EsfParameterStorageManagerCapabilities* capabilities) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (capabilities == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid argument: capabilities is NULL. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE("Retrieving storage capabilities");
    memset(capabilities, 0, sizeof(*capabilities));

    ret = EsfParameterStorageManagerStorageAdapterGetCapabilitiesPLStorage(
        capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get capabilities from PL Storage. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }

    // If there is another source for obtaining capabilities, call that API
    // here.

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Storage capabilities retrieved successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterGetItemCapabilities(
    EsfParameterStorageManagerItemID id,
    EsfParameterStorageManagerItemCapabilities* capabilities) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  do {
    if (id < 0 || kEsfParameterStorageManagerItemCustom <= id ||
        capabilities == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. id=%u, capabilities=%p. ret=%u(%s)", id,
          (void*)capabilities, ret, EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Retrieving item capabilities for id=%u", id);

    ret = EsfParameterStorageManagerStorageAdapterGetItemCapabilitiesPLStorage(
        id, capabilities);
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to get item capabilities from PL Storage. id=%u, ret=%u(%s)",
          id, ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    // If there is another source for obtaining capabilities, call that API
    // here.

    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Item capabilities retrieved successfully for id=%u", id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterClean(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret =
      EsfParameterStorageManagerStorageAdapterCleanPLStorage();
  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "EsfParameterStorageManagerStorageAdapterCleanPLStorage failed. "
        "ret=%u(%s)",
        ret, EsfParameterStorageManagerStrError(ret));
  }

  ret = EsfParameterStorageManagerStorageAdapterCleanOther();
  if (ret != kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_WARN(
        "EsfParameterStorageManagerStorageAdapterCleanOther failed. "
        "ret=%u(%s)",
        ret, EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return kEsfParameterStorageManagerStatusOk;
}

EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterDowngrade(void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  do {
    ret = EsfParameterStorageManagerStorageAdapterDowngradePLStorage();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "EsfParameterStorageManagerStorageAdapterDowngradePLStorage failed. "
          "ret=%u(%s)",
          ret, EsfParameterStorageManagerStrError(ret));
      break;
    }

    ret = EsfParameterStorageManagerStorageAdapterDowngradeOther();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_WARN(
          "EsfParameterStorageManagerStorageAdapterDowngradeOther failed. "
          "ret=%u(%s)",
          ret, EsfParameterStorageManagerStrError(ret));
      break;
    }
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %d(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterInit(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Initializing storage adapter");

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Initializing PL Storage");
    ret = EsfParameterStorageManagerStorageAdapterInitPLStorage();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to initialize PL Storage. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("PL Storage initialized successfully");

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Initializing Other storage area");
    ret = EsfParameterStorageManagerStorageAdapterInitOther();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to initialize the Other storage area. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Other storage area initialized successfully");
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

EsfParameterStorageManagerStatus EsfParameterStorageManagerStorageAdapterDeinit(
    void) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Deinitializing storage adapter");

  do {
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Deinitializing Other storage area");
    ret = EsfParameterStorageManagerStorageAdapterDeinitOther();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to deinitialize the Other storage area. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "Other storage area deinitialized successfully");

    ESF_PARAMETER_STORAGE_MANAGER_DEBUG("Deinitializing PL Storage");
    ret = EsfParameterStorageManagerStorageAdapterDeinitPLStorage();
    if (ret != kEsfParameterStorageManagerStatusOk) {
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Failed to deinitialize PL Storage. ret=%u(%s)", ret,
          EsfParameterStorageManagerStrError(ret));
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_DEBUG(
        "PL Storage deinitialized successfully");
  } while (0);
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterSaveInternal(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const uint8_t* data, uint32_t* save_size,
    EsfParameterStorageManagerWorkMemberData* member) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Saving data for item ID: %u, Storage ID: %u, Offset: %" PRIu32
      ", Size: %" PRIu32,
      id, storage_id, offset, size);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Saving to PL Storage");
      ret = EsfParameterStorageManagerStorageAdapterSavePLStorage(
          id, offset, size, data, save_size, member);
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Saving to Other Storage");
      ret = EsfParameterStorageManagerStorageAdapterSaveOther(
          id, offset, size, data, save_size, member);
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Unknown storage ID for item ID: %u",
                                          id);
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Successfully saved %" PRIu32 " bytes for item ID: %u", *save_size, id);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to save data for item ID: %u. ret=%u(%s)", id, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterValidateSave(
    EsfParameterStorageManagerItemID id, uint32_t offset, uint32_t size,
    const uint8_t* data, uint32_t* save_size,
    EsfParameterStorageManagerWorkMemberData* member) {
  (void)id;
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Validating save parameters for item ID: %u, Offset: %" PRIu32
      ", Size: %" PRIu32,
      id, offset, size);

  do {
    if (member == NULL || data == NULL || save_size == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. member=%p, data=%p, save_size=%p. ret=%u(%s)",
          (void*)member, (const void*)data, (void*)save_size, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (size == 0) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid size. Size cannot be 0 for item ID: %u. ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }

    if (member->storage.written_size < offset) {
      ret = kEsfParameterStorageManagerStatusOutOfRange;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset out of range. Written size: %" PRIu32 ", Offset: %" PRIu32
          " for item ID: %u. ret=%u(%s)",
          member->storage.written_size, offset, id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Save parameters validated successfully for item ID: %u", id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterLoadInternal(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, uint8_t* data, uint32_t* load_size) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;
  EsfParameterStorageManagerStorageID storage_id =
      EsfParameterStorageManagerStorageAdapterConvertItemIDToStorageID(id);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Loading data for item ID: %u, Storage ID: %u, Offset: %" PRIu32
      ", Size: %" PRIu32,
      id, storage_id, offset, size);

  switch (storage_id) {
    case kEsfParameterStorageManagerStoragePl: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Loading from PL Storage");
      ret = EsfParameterStorageManagerStorageAdapterLoadPLStorage(
          id, member, offset, size, data, load_size);
      break;
    }
    case kEsfParameterStorageManagerStorageOther: {
      ESF_PARAMETER_STORAGE_MANAGER_TRACE("Loading from Other Storage");
      ret = EsfParameterStorageManagerStorageAdapterLoadOther(
          id, member, offset, size, data, load_size);
      break;
    }
    case kEsfParameterStorageManagerStorageMax:
    default:
      ESF_PARAMETER_STORAGE_MANAGER_ERROR("Unknown storage ID for item ID: %u",
                                          id);
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
  }

  if (ret == kEsfParameterStorageManagerStatusOk) {
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Loaded data size: %" PRIu32 " for item ID: %u", *load_size, id);
  } else {
    ESF_PARAMETER_STORAGE_MANAGER_ERROR(
        "Failed to load data for item ID: %u. ret=%u(%s)", id, ret,
        EsfParameterStorageManagerStrError(ret));
  }

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}

static EsfParameterStorageManagerStatus
EsfParameterStorageManagerStorageAdapterValidateLoad(
    EsfParameterStorageManagerItemID id,
    const EsfParameterStorageManagerWorkMemberData* member, uint32_t offset,
    uint32_t size, uint8_t* data, uint32_t* load_size) {
  ESF_PARAMETER_STORAGE_MANAGER_TRACE("entry");
  EsfParameterStorageManagerStatus ret = kEsfParameterStorageManagerStatusOk;

  ESF_PARAMETER_STORAGE_MANAGER_TRACE(
      "Validating load parameters for item ID: %u, Offset: %" PRIu32
      ", Size: %" PRIu32,
      id, offset, size);

  do {
    if (member == NULL || data == NULL || load_size == NULL) {
      ret = kEsfParameterStorageManagerStatusInternal;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid arguments. member=%p, data=%p, load_size=%p for item ID: "
          "%u. ret=%u(%s)",
          (const void*)member, (void*)data, (void*)load_size, id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_LOGIC_ERROR);
      break;
    }
    if (size == 0) {
      ret = kEsfParameterStorageManagerStatusInvalidArgument;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Invalid size. Size cannot be 0 for item ID: %u. ret=%u(%s)", id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    if (member->storage.written_size < offset) {
      ret = kEsfParameterStorageManagerStatusOutOfRange;
      ESF_PARAMETER_STORAGE_MANAGER_ERROR(
          "Offset out of range. Written size: %" PRIu32 ", Offset: %" PRIu32
          " for item ID: %u. ret=%u(%s)",
          member->storage.written_size, offset, id, ret,
          EsfParameterStorageManagerStrError(ret));
      ESF_PARAMETER_STORAGE_MANAGER_EVENT_ERROR(
          ESF_PARAMETER_STORAGE_MANAGER_EVENT_ID_INVALID_PARAMETER_FAILURE);
      break;
    }
    ESF_PARAMETER_STORAGE_MANAGER_TRACE(
        "Load parameters validated successfully for item ID: %u", id);
  } while (0);

  ESF_PARAMETER_STORAGE_MANAGER_TRACE("exit %u(%s)", ret,
                                      EsfParameterStorageManagerStrError(ret));
  return ret;
}
