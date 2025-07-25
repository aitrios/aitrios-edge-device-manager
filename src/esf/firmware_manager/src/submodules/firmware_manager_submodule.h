/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_SRC_SUBMODULE_FIRMWARE_MANGER_SUBMODULE_H_
#define ESF_FIRMWARE_MANAGER_SRC_SUBMODULE_FIRMWARE_MANGER_SUBMODULE_H_

#include "firmware_manager.h"
#include "memory_manager.h"
#include "sensor_ai_lib/sensor_ai_lib_fwupdate.h"

typedef void *EsfFwMgrSubmoduleHandle;

typedef struct EsfFwMgrSubmodulePrepareWriteRequest {
  int32_t total_size;
  EsfMemoryManagerHandle internal_buffer_handle;
  int32_t internal_buffer_size;
} EsfFwMgrSubmodulePrepareWriteRequest;

// Before calling the submodule's open function, the core block initiates a
// dummy firmware update using the Sensor AI Library to prevent other modules
// from starting streaming. However, some submodules use the Sensor AI Library
// to perform the actual update. In that case, the submodule must stop the
// dummy update before starting the actual update, since the Sensor AI Library
// can only handle one update at a time. Additionally, the core block needs to
// know whether the submodule has stopped the dummy update to avoid stopping
// it twice. The struct EsfFwMgrDummyUpdateHandleInfo is used to pass the
// information required to facilitate this process.
typedef struct EsfFwMgrDummyUpdateHandleInfo {
  // [in] The handle to stop the dummy FW update started by the core block.
  SsfSensorLibFwUpdateHandle handle;
  // [in/out] If the submodule stops the dummy FW update, set `canceled` to
  // true.
  bool canceled;
} EsfFwMgrDummyUpdateHandleInfo;

typedef struct EsfFwMgrSubmoduleOps {
  EsfFwMgrResult (*init)(void);
  EsfFwMgrResult (*deinit)(void);
  bool (*is_supported)(EsfFwMgrTarget target);
  EsfFwMgrResult (*open)(
      const EsfFwMgrOpenRequest *request,
      const EsfFwMgrSubmodulePrepareWriteRequest *prepare_write,
      EsfFwMgrDummyUpdateHandleInfo *dummy_update_handle_info,
      EsfFwMgrSubmoduleHandle *handle, int32_t *writable_size);
  EsfFwMgrResult (*close)(EsfFwMgrSubmoduleHandle handle, bool aborted);
  EsfFwMgrResult (*write)(EsfFwMgrSubmoduleHandle handle,
                          const EsfFwMgrWriteRequest *request);
  EsfFwMgrResult (*post_process)(EsfFwMgrSubmoduleHandle handle);
  EsfFwMgrResult (*erase)(EsfFwMgrSubmoduleHandle handle);
  EsfFwMgrResult (*get_binary_header_info)(EsfFwMgrSubmoduleHandle handle,
                                           EsfFwMgrBinaryHeaderInfo *info);
  EsfFwMgrResult (*get_info)(EsfFwMgrGetInfoData *data);
} EsfFwMgrSubmoduleOps;

#endif  // ESF_FIRMWARE_MANAGER_SRC_SUBMODULE_FIRMWARE_MANGER_SUBMODULE_H_
