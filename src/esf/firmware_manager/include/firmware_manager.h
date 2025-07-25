/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_H_
#define ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_H_

#include <stdbool.h>
#include <stdint.h>

#define ESF_FIRMWARE_MANAGER_HANDLE_INVALID (NULL)
typedef struct EsfFwMgrContext* EsfFwMgrHandle;

typedef enum EsfFwMgrResult {
  kEsfFwMgrResultOk,                  // No errors.
  kEsfFwMgrResultInvalidArgument,     // Invalid argument
  kEsfFwMgrResultFailedPrecondition,  // Invalid state of Firmware Manager
  kEsfFwMgrResultAborted,             // Failed to Update/erase data.
  kEsfFwMgrResultOutOfRange,          // Parameter is out of valid range.
  kEsfFwMgrResultResourceExhausted,   //
  kEsfFwMgrResultCancelled,
  kEsfFwMgrResultUnavailable,
  kEsfFwMgrResultUnimplemented,
  kEsfFwMgrResultInternal,
  kEsfFwMgrResultBusy,
  kEsfFwMgrResultInvalidData,
} EsfFwMgrResult;

// To: caller, The values below may be changed in the feature.
// To: implementer: Do NOT change the values until all the callers do not depend
// on them.
typedef enum EsfFwMgrTarget {
  kEsfFwMgrTargetSensorLoader = 0,
  kEsfFwMgrTargetSensorFirmware = 1,
  kEsfFwMgrTargetProcessorLoader = 2,
  kEsfFwMgrTargetProcessorFirmware = 3,
  kEsfFwMgrTargetSensorCalibrationParam = 5,
  kEsfFwMgrTargetAIModel = 9,
  kEsfFwMgrTargetInvalid,
} EsfFwMgrTarget;

// Init/Deinit -----------------------------------------------------------------
EsfFwMgrResult EsfFwMgrInit(void);
EsfFwMgrResult EsfFwMgrDeinit(void);

// Open ------------------------------------------------------------------------
// + 1 is for the null character at the end.
#define ESF_FIRMWARE_MANAGER_TARGET_NAME_SIZE (32 + 1)
#define ESF_FIRMWARE_MANAGER_TARGET_VERSION_SIZE (44 + 1)
#define ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE (32)

typedef struct EsfFwMgrOpenRequest {
  EsfFwMgrTarget target;
  char name[ESF_FIRMWARE_MANAGER_TARGET_NAME_SIZE];
  char version[ESF_FIRMWARE_MANAGER_TARGET_VERSION_SIZE];
  uint8_t hash[ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE];
} EsfFwMgrOpenRequest;

typedef struct EsfFwMgrPrepareWriteRequest {
  int32_t total_size;
  int32_t memory_size;
} EsfFwMgrPrepareWriteRequest;

typedef struct EsfFwMgrPrepareWriteResponse {
  int32_t memory_size;
  int32_t writable_size;
} EsfFwMgrPrepareWriteResponse;

typedef struct EsfFwMgrOpenResponse {
  EsfFwMgrHandle handle;
  EsfFwMgrPrepareWriteResponse prepare_write;
} EsfFwMgrOpenResponse;

EsfFwMgrResult EsfFwMgrOpen(const EsfFwMgrOpenRequest* request,
                            const EsfFwMgrPrepareWriteRequest* prepare_write,
                            EsfFwMgrOpenResponse* response);

// Close -----------------------------------------------------------------------
EsfFwMgrResult EsfFwMgrClose(EsfFwMgrHandle handle);

// CopyToInternalBuffer --------------------------------------------------------
typedef struct EsfFwMgrCopyToInternalBufferRequest {
  int32_t offset;
  int32_t size;
  const uint8_t* data;
} EsfFwMgrCopyToInternalBufferRequest;

EsfFwMgrResult EsfFwMgrCopyToInternalBuffer(
    EsfFwMgrHandle handle, const EsfFwMgrCopyToInternalBufferRequest* request);

// Write -----------------------------------------------------------------------
typedef struct EsfFwMgrWriteRequest {
  int32_t offset;
  int32_t size;
} EsfFwMgrWriteRequest;

EsfFwMgrResult EsfFwMgrWrite(EsfFwMgrHandle handle,
                             const EsfFwMgrWriteRequest* request);

// Post process ----------------------------------------------------------------
EsfFwMgrResult EsfFwMgrPostProcess(EsfFwMgrHandle handle);

// Erase -----------------------------------------------------------------------
EsfFwMgrResult EsfFwMgrErase(EsfFwMgrHandle handle);

// GetBinaryHeader -------------------------------------------------------------
typedef enum EsfFwMgrSwArchVersion {
  kEsfFwMgrSwArchVersion1,
  kEsfFwMgrSwArchVersion2,
  kEsfFwMgrSwArchVersionUnknown,
} EsfFwMgrSwArchVersion;

typedef struct EsfFwMgrBinaryHeaderInfo {
  EsfFwMgrSwArchVersion sw_arch_version;
} EsfFwMgrBinaryHeaderInfo;

EsfFwMgrResult EsfFwMgrGetBinaryHeaderInfo(EsfFwMgrHandle handle,
                                           EsfFwMgrBinaryHeaderInfo* info);

// Get Info --------------------------------------------------------------------
#define ESF_FIRMWARE_MANAGER_AI_MODEL_SLOT_NUM \
  CONFIG_EXTERNAL_FIRMWARE_MANAGER_AI_MODEL_SLOT_NUM

// + 1 is for the null character at the end.
#define ESF_FIRMWARE_MANAGER_LAST_UPDATE_SIZE (32 + 1)

typedef struct EsfFwMgrGetInfoResponse {
  char version[ESF_FIRMWARE_MANAGER_TARGET_VERSION_SIZE];
  char last_update[ESF_FIRMWARE_MANAGER_LAST_UPDATE_SIZE];
  uint8_t hash[ESF_FIRMWARE_MANAGER_TARGET_HASH_SIZE];
} EsfFwMgrGetInfoResponse;

typedef struct EsfFwMgrGetInfoData {
  EsfFwMgrTarget target;
  char name[ESF_FIRMWARE_MANAGER_TARGET_NAME_SIZE];
  int32_t in_length;
  EsfFwMgrGetInfoResponse* response;
  int32_t out_length;
} EsfFwMgrGetInfoData;

EsfFwMgrResult EsfFwMgrGetInfo(EsfFwMgrGetInfoData* data);

// Factory Reset ---------------------------------------------------------------
typedef enum {
  kEsfFwMgrResetCauseButton,
  kEsfFwMgrResetCauseCommand,
  kEsfFwMgrResetCauseDowngrade,
} EsfFwMgrFactoryResetCause;

EsfFwMgrResult EsfFwMgrStartFactoryReset(EsfFwMgrFactoryResetCause cause);

// [DEPRECATED] This function will be removed soon.
// It is kept here temporarily to support backward compatibility.
// It does nothing and always returns kEsfFwMgrResultOk.
EsfFwMgrResult EsfFwMgrSetFactoryResetFlag(bool factory_reset_flag);

// [DEPRECATED] This function will be removed soon.
// It is kept here temporarily to support backward compatibility.
// It always sets factory_reset_flag true.
EsfFwMgrResult EsfFwMgrGetFactoryResetFlag(bool* factory_reset_flag);

// Switch firmware slot --------------------------------------------------------
EsfFwMgrResult EsfFwMgrSwitchProcessorFirmwareSlot(void);

#endif  // ESF_FIRMWARE_MANAGER_INCLUDE_FIRMWARE_MANGER_H_
