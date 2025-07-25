/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_LOG_MANAGER_LOG_MANAGER_H_
#define ESF_LOG_MANAGER_LOG_MANAGER_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define DLOG_IS_SAVING_TO_FLASH_ENABLED \
  (CONFIG_EXTERNAL_LOG_MANAGER_DLOG_FLASH_ENABLE)
#define DLOG_IS_EXCEPTION_UPLOAD_ENABLED \
  (CONFIG_EXTERNAL_LOG_MANAGER_EXCEPTION_UPLOAD_ENABLE)

#define ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE (65)
#define ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE (257)
#define ESF_LOG_MANAGER_BLOCK_TYPE_MAX_NUM (4)
#define ESF_LOG_DATATIME_SIZE (25)

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kEsfLogManagerStateInvalid,  // Invalid
  kEsfLogManagerStateInit,     // Init
  kEsfLogManagerStateStart,    // Start
  kEsfLogManagerStateNum       // EsfLogManagerState element count
} EsfLogManagerState;

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kEsfLogManagerStatusOk,          //  No errors.
  kEsfLogManagerStatusFailed,      //  Status Error.
  kEsfLogManagerStatusParamError,  //  Parameter Error.
  kEsfLogManagerStatusNum          //  EsfLogManagerStatus element count
} EsfLogManagerStatus;

// This code enumeration type that defines the output destination of Dlog.
typedef enum {
  kEsfLogManagerDlogDestUart,   // UART output
  kEsfLogManagerDlogDestStore,  // Store in memory and upload to the cloud
  kEsfLogManagerDlogDestBoth,   // UART output & Store in memory and upload to
                                // the cloud
  kEsfLogManagerDlogDestNum     // UtilitiesLogDest element count
} EsfLogManagerDlogDest;

// This code enumeration type that defines the log level of Dlog.
typedef enum {
  kEsfLogManagerDlogLevelCritical,  // Critical
  kEsfLogManagerDlogLevelError,     // Error
  kEsfLogManagerDlogLevelWarn,      // Warning
  kEsfLogManagerDlogLevelInfo,      // Info
  kEsfLogManagerDlogLevelDebug,     // Debug
  kEsfLogManagerDlogLevelTrace,     // Trace
  kEsfLogManagerDlogLevelNum        // EsfLogManagerDlogLevel element count
} EsfLogManagerDlogLevel;

// This code enumeration type that defines the log level of Elog.
typedef enum {
  kEsfLogManagerElogLevelCritical,  // Critical
  kEsfLogManagerElogLevelError,     // Error
  kEsfLogManagerElogLevelWarn,      // Warning
  kEsfLogManagerElogLevelInfo,      // Info
  kEsfLogManagerElogLevelDebug,     // Debug
  kEsfLogManagerElogLevelTrace,     // Trace
  kEsfLogManagerElogLevelNum        // EsfLogManagerElogLevel element count
} EsfLogManagerElogLevel;

// This code structure that defines the message of Elog
typedef struct {
  EsfLogManagerElogLevel elog_level;  // Elog Log Level
  char time[ESF_LOG_DATATIME_SIZE];   // Timestamp
  uint32_t component_id;              // Component ID
  uint32_t event_id;                  // Event ID
} EsfLogManagerElogMessage;

// This code enumeration type that defines the set parameter block type
typedef enum {
  kEsfLogManagerBlockTypeSysApp,  // SysApp Setting
  kEsfLogManagerBlockTypeEdgeApp =
      kEsfLogManagerBlockTypeSysApp,  // EdgeApp Setting
  kEsfLogManagerBlockTypeSensor,      // Sensor Setting
  kEsfLogManagerBlockTypeAiisp,       // Aiisp Setting
  kEsfLogManagerBlockTypeVicapp,      // Vicapp Setting
  kEsfLogManagerBlockTypeAll,         // All Setting(SysApp/EdgeApp/Sensor)
  kEsfLogManagerBlockTypeNum          // EsfLogManagerSettingBlock element count
} EsfLogManagerSettingBlockType;

// This code enumerated type that defines the memory type.
typedef enum {
  kEsfLogManagerMemoryTypeCurrentRAM,  // Current RAM
  kEsfLogManagerMemoryTypeFullRAM,     // Full RAM
  kEsfLogManagerMemoryTypeFlash,       // Flash
  kEsfLogManagerMemoryTypeNum          // kEsfLogManagerMemoryType element count
} EsfLogManagerMemoryType;

// This code enumeration type that defines whether encryption is enabled or not.
typedef enum {
  kEsfLogManagerEncryptDisable,  // Encryption disabled
  kEsfLogManagerEncryptEnable,   // Encryption enabled
  kEsfLogManagerEncryptNum       // kEsfLogManagerEncrypt element count
} EsfLogManagerEncrypt;

// A structure that defines the data acquisition result.
typedef struct EsfLogManagerResultInfo {
  EsfLogManagerStatus status;    // Processing result
  EsfLogManagerMemoryType type;  // Data storage type
  uint32_t size;                 // Data storage size
  uint8_t *buf;                  // Data storage pointer address
} EsfLogManagerResultInfo;

// This code enumeration type that setting details for device settings
typedef struct EsfLogManagerParameterMask {
  uint8_t dlog_dest : 1;     // The type of input/output settings to
                             // DeviceSetting is Dlog
  uint8_t dlog_level : 1;    // The type of input/output settings to
                             // DeviceSetting is DlogLevel
  uint8_t elog_level : 1;    // The type of input/output settings to
                             // DeviceSetting is ElogLevel
  uint8_t dlog_filter : 1;   // The type of input/output settings to
                             // DeviceSetting is LogFilter
  uint8_t storage_name : 1;  // The type of input/output settings to
                             // DeviceSetting is storage_name
  uint8_t storage_path : 1;  // The type of input/output settings to
                             // DeviceSetting is storage_path
} EsfLogManagerParameterMask;

typedef struct EsfLogManagerParameterValue {
  EsfLogManagerDlogDest dlog_dest;    // Set the Dlog output destination
  EsfLogManagerDlogLevel dlog_level;  // Set the Dlog output level
  EsfLogManagerElogLevel elog_level;  // Set the Elog output level
  uint32_t dlog_filter;               // Set the Dlog filter
  char storage_name[ESF_LOG_MANAGER_STORAGE_NAME_MAX_SIZE];  // Set the storage
                                                             // name
  char
      storage_path[ESF_LOG_MANAGER_STORAGE_SUB_DIR_PATH_MAX_SIZE];  // Set the
                                                                    // storage
                                                                    // path name
} EsfLogManagerParameterValue;

// A structure that notifies when the Dlog level changes
typedef struct EsfLogManagerDlogChangeInfo {
  EsfLogManagerParameterValue value;  // Dlog level
  uint32_t module_id;                 // module_id
} EsfLogManagerDlogChangeInfo;

// Parameters to be passed when executing the callback
typedef void (*EsfLogManagerChangeDlogCallback)(
    EsfLogManagerDlogChangeInfo *info);

typedef void (*EsfLogManagerBulkDlogCallback)(size_t size, void *user_data);
// This code structure that defines buffer configuration information.
// (size, number of sides)
typedef struct EsfLogManagerLogBufferInfo {
  // Size of one buffer side
  uint32_t size;

  // Number of buffer sides
  uint32_t num;
} EsfLogManagerLogBufferInfo;

// This code structure that defines LogManager configuration information.
// (number of buffers, etc.).
typedef struct EsfLogManagerLogInfo {
  // Dlog RAM buffer configuration information
  EsfLogManagerLogBufferInfo dlog_ram;

  // Flash buffer configuration information for Dlog
  EsfLogManagerLogBufferInfo dlog_flash;

  // Elog RAM buffer configuration information
  EsfLogManagerLogBufferInfo elog_ram;

  // Flash buffer configuration information for Elog
  EsfLogManagerLogBufferInfo elog_flash;
} EsfLogManagerLogInfo;

// """Initialize LogManager

// The LogManager is initialized and the LogManager state transitions to active.
// In case of an error, no state transition takes place.
// This API can be called multiple times.
// If you want to recall the message normally, please be sure to
// EsfLogManagerDeinit the message.

// Args:
//    no arguments

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
//     If memory allocation, Flash access, thread generation, etc. fail,
//     and LogManager cannot be started.

// """

EsfLogManagerStatus EsfLogManagerInit(void);
// """Start LogManager
// The initialization of the LogManager, allocation of memory for Dlog,
// and generation of threads will be performed.

// Args:
//    no arguments

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
//     If memory allocation, Flash access, thread generation, etc. fail,
//     and LogManager cannot be started.

// """
EsfLogManagerStatus EsfLogManagerStart(void);
// """Performs LogManager termination processing

// The LogManager will be finalized and the state will change to inactive.
// In case of an error, no state transition takes place.
// When Flash storage is enabled, the Dlog and Elog data being accumulated
// in RAM is saved to each Flash area using the API provided by HAL.
// If Flash saving is disabled, the Dlog and Elog data being stored in RAM
// will be discarded. Therefore, if necessary, before executing this API,
// call the API to obtain the stored data, and then execute it after
// obtaining the data.
// As of 2024/08/01 Flash saving is T.B.D. Multiple calls to this
// API are prohibited.

// Args:
//    no arguments

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination.
//     If LogManager cannot be terminated due to failure in releasing
//     resources such as Flash access or thread deletion.

// """

EsfLogManagerStatus EsfLogManagerDeinit(void);

// """Set the LogManager parameters.

// Specifies the following processing when a Dlog/Elog output request
// is made. For details on the settings, please refer to the log settings.
// - Specify the Dlog output destination
// - DLog level specification
// - ELog level specification
// - Dlog filter specification

// Args:
//    block_type(EsfLogManagerSettingBlockType): Block type for parameter
//    setting value(EsfLogManagerParameterValue ): Parameter value to be set
//    mask(EsfLogManagerParameterMask): Mask value of the parameter to be set

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: If the argument type is invalid
//    kEsfLogManagerStatusFailed: abnormal termination
//                                If you cannot save to Flash

// """

EsfLogManagerStatus EsfLogManagerSetParameter(
    const EsfLogManagerSettingBlockType block_type,
    const EsfLogManagerParameterValue value,
    const EsfLogManagerParameterMask mask);

// """Gets the LogManager parameter settings.

// Gets the setting value set in LogManager when requesting Dlog/Elog output.
// Please refer to EsfLogManagerParameterValue for the available settings.

// Args:
//     block_type(EsfLogManagerSettingBlockType): Block type to get parameters
//     value (EsfLogManagerParameterValue ): This is a structure that stores
//       the setting value set in LogManager when a Dlog/Elog output request is
//       made.

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: If the argument type is invalid
//    kEsfLogManagerStatusFailed: abnormal termination
//                                If the setting acquisition process fails

// """

EsfLogManagerStatus EsfLogManagerGetParameter(
    EsfLogManagerSettingBlockType block_type,
    EsfLogManagerParameterValue *value);

// """Gets the LogManager parameter settings.

// Gets the setting value set in LogManager when requesting Dlog/Elog output.
// Please refer to EsfLogManagerParameterValue for the available settings.

// Args:
//     module_id(uint32_t): Module id to get parameters
//     value (EsfLogManagerParameterValue ): This is a structure that stores
//       the setting value set in LogManager when a Dlog/Elog output request is
//       made.

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: If the argument type is invalid
//    kEsfLogManagerStatusFailed: abnormal termination
//                                If the setting acquisition process fails

// """

EsfLogManagerStatus EsfLogManagerGetModuleParameter(
    uint32_t module_id, EsfLogManagerParameterValue *value);

// """Requests the Dlog to be stored in the LogManager

// The string specified by the string pointer is stored in the Dlog memory
// in the specified size.
// When the accumulated memory for DLog becomes full, a callback notification
// of DLog memory full will be sent.

// Args:
//    *str (uint8_t): Specifies the string pointer to be accumulated.
//    size (uint32_t): Specifies the string size.

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: If the argument type is invalid
//    kEsfLogManagerStatusFailed: abnormal termination
//                                If the accumulation process fails

// """

EsfLogManagerStatus EsfLogManagerStoreDlog(uint8_t *str, uint32_t size);

//""" Requests the Elog to send to LogManager

// Args:
//    *message (EsfLogManagerElogMessage): Specifies the struct pointer of Elog

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParmError: If the argument type is invalid
//    kEsfLogManagerStatusFailed: abnormal termination
//                                If the accumulation process fails

// """

EsfLogManagerStatus EsfLogManagerSendElog(
    const EsfLogManagerElogMessage *message);

// """This function obtains LogManager configuration information such as
// Dlog RAM buffer size.

// Obtains LogManager configuration information (buffer size, etc.).
// An error will occur if the argument log_info is NULL.
// This API can be called multiple times.

// Args:
//    *log_info (EsfLogManagerLogInfo): This is the configuration information
//      for LogManager. The buffer size, number of faces, etc. are stored in
//        the structure shown in 4.2.13. An error will occur if NULL is
//        specified.

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: If the arguments buf and out_size are NULL
// """

EsfLogManagerStatus EsfLogManagerGetLogInfo(
    struct EsfLogManagerLogInfo *log_info);

// """Register the callback function that notifies when the Dlog settings have
//  been changed.

// Notifications will be sent about the modifications when the Dlog settings
// are changed using EsfLogManagerSetParameter() for the group belonging to
// the module ID where the callback was registered.
// Args:
//     module_id (uint32_t): The module ID for which the settings were changed
//     *callback (EsfLogManagerChangeDlogCallback): The modified parameters
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: abnormal termination
//                                If the accumulation process fails
// """

EsfLogManagerStatus EsfLogManagerRegisterChangeDlogCallback(
    uint32_t module_id, EsfLogManagerChangeDlogCallback callback);

// """Cancel the callback that notifies when the Dlog settings are changed.

// Cancel the callback registration for the module ID specified as an argument.
// Args:
//     module_id (uint32_t): The module ID for which the registration is to be
//     canceled
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: abnormal termination
//                                If the accumulation process fails
// """

EsfLogManagerStatus EsfLogManagerUnregisterChangeDlogCallback(
    uint32_t module_id);

// """Transfer the specified buffer to the blob.

// Transfer the specified buffer to the Blob with the specified size.
// Args:
//     module_id(uint32_t): Module id
//     size (size_t): Transfer data size
//     *bulk_log(uint8_t): Transfer data address
//     callback(EsfLogManagerBulkDlogCallback): The function to notify
//     processing results *user_data(void): user data
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: abnormal termination
//                                If the accumulation process fails
// """
EsfLogManagerStatus EsfLogManagerSendBulkDlog(
    uint32_t module_id, size_t size, uint8_t *bulk_log,
    EsfLogManagerBulkDlogCallback callback, void *user_data);

// """This function gets the exception occurrence information such as
// the PC register and stack at the time the exception occurred.

// The stringified Exception data is stored in the buf argument.
// If an error occurs, no data will be stored in the buf argument.
// This API can be called multiple times.

// Args:
//    size (uint32_t): Please specify the buffer size for the exception
//      data obtained from the LogManager configuration information.
//      If the data size is less than the above, the data will not be
//      stored in the following buffer.
//    *buf (uint8_t): Exception data storage buffer. If NULL or the data
//       size is less than the above, an error is returned.
//    *out_size (uint32_t): Returns the size of the exception data actually
//       stored.If the error or exception data size is 0, it returns 0.
//       If an error occurs, no data is stored in the buffer.

// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusParamError: If the arguments buf and out_size are NULL
//    kEsfLogManagerStatusFailed: abnormal termination
//                                Failed to access Exception data, buf is
//                                smaller than Exception data

EsfLogManagerStatus EsfLogManagerGetExceptionData(uint32_t size, uint8_t *buf,
                                                  uint32_t *out_size);

#ifdef __cplusplus
}
#endif
#endif  // ESF_LOG_MANAGER_LOG_MANAGER_H_
