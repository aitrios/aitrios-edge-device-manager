/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Define the external public API for SystemManager.

#ifndef ESF_SYSTEM_MANAGER_INCLUDE_SYSTEM_MANAGER_H_
#define ESF_SYSTEM_MANAGER_INCLUDE_SYSTEM_MANAGER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESF_SYSTEM_MANAGER_DEVICE_MANIFEST_MAX_SIZE (32768)
#define ESF_SYSTEM_MANAGER_DPS_URL_MAX_SIZE (256)
#define ESF_SYSTEM_MANAGER_COMMON_NAME_MAX_SIZE (256)
#define ESF_SYSTEM_MANAGER_DPS_SCOPE_ID_MAX_SIZE (17)
#define ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE (33)
#define ESF_SYSTEM_MANAGER_REGISTER_TOKEN_MAX_SIZE (45)
#define ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE (65)
#define ESF_SYSTEM_MANAGER_EVP_HUB_PORT_MAX_SIZE (6)
#define ESF_SYSTEM_MANAGER_IOT_PLATFORM_MAX_SIZE (33)
#define ESF_SYSTEM_MANAGER_ROOT_CA_MAX_SIZE (393216)
#define ESF_SYSTEM_MANAGER_ROOT_CA_HASH_MAX_SIZE (512)

// TODO: Permanent support Hw Info max size.
#define ESF_SYSTEM_MANAGER_HWINFO_MODEL_NAME_MAX_SIZE (33)
#define ESF_SYSTEM_MANAGER_HWINFO_MANUFACTURER_NAME_MAX_SIZE (33)
#define ESF_SYSTEM_MANAGER_HWINFO_PRODUCT_SERIAL_NUMBER_MAX_SIZE (33)
#define ESF_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE (64)
#define ESF_SYSTEM_MANAGER_HWINFO_AIISP_CHIP_ID_MAX_SIZE (37)
#define ESF_SYSTEM_MANAGER_HWINFO_SENSOR_ID_MAX_SIZE (37)
#define ESF_SYSTEM_MANAGER_HWINFO_APP_PROCESSOR_TYPE_MAX_SIZE (64)
#define ESF_SYSTEM_MANAGER_HWINFO_SENSOR_MODEL_NAME_MAX_SIZE (64)
#define ESF_SYSTEM_MANAGER_HWINFO_DEVICE_NAME_MAX_SIZE (33)

// This code defines an enumeration type for the result of executing an API.
typedef enum {
  kEsfSystemManagerResultOk,             // OK.
  kEsfSystemManagerResultParamError,     // Parameter error.
  kEsfSystemManagerResultInternalError,  // Internal error.
  kEsfSystemManagerResultOutOfRange,     // Out of range error.
  kEsfSystemManagerResultEmptyData       // Empty data error.
} EsfSystemManagerResult;

// This code defines an enumeration type for the EVP TLS.
typedef enum {
  kEsfSystemManagerEvpTlsDisable,  // Disable EVP TLS.
  kEsfSystemManagerEvpTlsEnable    // Enable EVP TLS.
} EsfSystemManagerEvpTlsValue;

// This code defines an enumeration type for the HW Info.
typedef struct EsfSystemManagerHwInfo {
  char
      model_name[ESF_SYSTEM_MANAGER_HWINFO_MODEL_NAME_MAX_SIZE];  // Model Name.
  char manufacturer_name
      [ESF_SYSTEM_MANAGER_HWINFO_MANUFACTURER_NAME_MAX_SIZE];  // Manufacturer
                                                               // Name.
  char product_serial_number
      [ESF_SYSTEM_MANAGER_HWINFO_PRODUCT_SERIAL_NUMBER_MAX_SIZE];  // Product
                                                                   // Serial
                                                                   // Number.
  char serial_number
      [ESF_SYSTEM_MANAGER_HWINFO_SERIAL_NUMBER_MAX_SIZE];  // Serial Number.
  char aiisp_chip_id
      [ESF_SYSTEM_MANAGER_HWINFO_AIISP_CHIP_ID_MAX_SIZE];  // AIISP Chip ID.
  char sensor_id[ESF_SYSTEM_MANAGER_HWINFO_SENSOR_ID_MAX_SIZE];  // Sensor ID.
  char app_processor_type
      [ESF_SYSTEM_MANAGER_HWINFO_APP_PROCESSOR_TYPE_MAX_SIZE];  // Application
                                                                // Processor
                                                                // Type.
  char sensor_model_name
      [ESF_SYSTEM_MANAGER_HWINFO_SENSOR_MODEL_NAME_MAX_SIZE];  // Sensor Model
                                                               // Name.
  char device_name[ESF_SYSTEM_MANAGER_HWINFO_DEVICE_NAME_MAX_SIZE];  // DeviceName
} EsfSystemManagerHwInfo;

// This code defines an enumeration type for the InitialSettingFlag.
typedef enum {
  kEsfSystemManagerInitialSettingNotCompleted,  // Initial setting not
                                                // completed.
  kEsfSystemManagerInitialSettingCompleted      // Initial setting completed.
} EsfSystemManagerInitialSettingFlag;

// This code defines an enumeration type for the reset causes.
typedef enum {
  kEsfSystemManagerResetCauseUnknown = -1,
  kEsfSystemManagerResetCauseSysChipPowerOnReset = 0,
  kEsfSystemManagerResetCauseSysBrownOut,
  kEsfSystemManagerResetCauseCoreSoft,
  kEsfSystemManagerResetCauseCoreDeepSleep,
  kEsfSystemManagerResetCauseWDT,
  kEsfSystemManagerResetCauseSoftResetNormal,
  kEsfSystemManagerResetCauseSoftResetError,
  kEsfSystemManagerResetCauseDefault,
  kEsfSystemManagerResetCauseClear,
  kEsfSystemManagerResetCauseMax
} EsfSystemManagerResetCause;

// This code defines an enumeration type for the EVP reset causes.
typedef enum {
  kEsfSystemManagerEvpResetCauseClear,
  kEsfSystemManagerEvpResetCauseMemoryAllocFailure,
  kEsfSystemManagerEvpResetCauseFreezeDetection,
  kEsfSystemManagerEvpResetCauseMax
} EsfSystemManagerEvpResetCause;

// This code defines an enumeration type for the system manager reboot types.
typedef enum {
  kEsfSystemManagerRebootTypeSystemNormal,
  kEsfSystemManagerRebootTypeSystemAbnormal,
  kEsfSystemManagerRebootTypeEvpMemoryAllocFailure,
  kEsfSystemManagerRebootTypeEvpFreezeDetection,
  kEsfSystemManagerRebootTypeMax
} EsfSystemManagerRebootType;

// """Retrieves the Device Manifest from the parameter storage manager.
// This function retrieves the Device Manifest data from the parameter storage
// manager. The function fills the provided buffer with the Device Manifest data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Device Manifest data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Device Manifest data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Device Manifest data.
// Returns:
//   kEsfSystemManagerResultOk: The Device Manifest was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
//   kEsfSystemManagerResultEmptyData: Device Manifest data is empty.
// """
EsfSystemManagerResult EsfSystemManagerGetDeviceManifest(char *data,
                                                         size_t *data_size);

// """Retrieves the Dps URL from the parameter storage manager.
// This function retrieves the Dps URL data from the parameter storage
// manager. The function fills the provided buffer with the Dps URL data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Dps URL data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Dps URL data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Dps URL data.
// Returns:
//   kEsfSystemManagerResultOk: The Dps URL was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetDpsUrl(char *data, size_t *data_size);

// """Sets the Dps URL in the parameter storage manager.
// This function sets the Dps URL in the parameter storage manager.
// - The provided Dps URL data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
//  Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     Dps URL. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the Dps URL data. It must be greater
//     than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The Dps URL was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetDpsUrl(const char *data,
                                                 size_t data_size);

// """Retrieves the Common Name from the parameter storage manager.
// This function retrieves the Common Name data from the parameter storage
// manager. The function fills the provided buffer with the Common Name data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Common Name data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Common Name data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Common Name data.
// Returns:
//   kEsfSystemManagerResultOk: The Common Name was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetCommonName(char *data,
                                                     size_t *data_size);

// """Sets the Common Name in the parameter storage manager.
// This function sets the Common Name in the parameter storage manager.
// - The provided Common Name data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
//  Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     Common Name. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the Common Name data. It must be greater
//     than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The Common Name was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetCommonName(const char *data,
                                                     size_t data_size);

// """Retrieves the Dps Scope ID from the parameter storage manager.
// This function retrieves the Dps Scope ID data from the parameter storage
// manager. The function fills the provided buffer with the Dps Scope ID data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Dps Scope ID data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Dps Scope ID data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Dps Scope ID data.
// Returns:
//   kEsfSystemManagerResultOk: The Dps Scope ID was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetDpsScopeId(char *data,
                                                     size_t *data_size);

// """Sets the Dps Scope ID in the parameter storage manager.
// This function sets the Dps Scope ID in the parameter storage manager.
// - The provided Dps Scope ID data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
//  Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     Dps Scope ID. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the Dps Scope ID data. It must be greater
//     than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The Dps Scope ID was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetDpsScopeId(const char *data,
                                                     size_t data_size);

// """Retrieves the Project ID from the parameter storage manager.
// This function retrieves the Project ID data from the parameter storage
// manager. The function fills the provided buffer with the Project ID data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Project ID data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Project ID data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Project ID data.
// Returns:
//   kEsfSystemManagerResultOk: The Project ID was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetProjectId(char *data,
                                                    size_t *data_size);

// """Sets the Project ID in the parameter storage manager.
// This function sets the Project ID in the parameter storage manager.
// - The provided Project ID data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
//  Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     Project ID. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the Project ID data. It must be greater
//     than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The Project ID was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetProjectId(const char *data,
                                                    size_t data_size);

// """Retrieves the Register Token from the parameter storage manager.
// This function retrieves the Register Token data from the parameter storage
// manager. The function fills the provided buffer with the Register Token data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Register Token data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Register Token data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Register Token data.
// Returns:
//   kEsfSystemManagerResultOk: The Register Token was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetRegisterToken(char *data,
                                                        size_t *data_size);

// """Sets the Register Token in the parameter storage manager.
// This function sets the Register Token in the parameter storage manager.
// - The provided Register Token data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
// Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     Register Token. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the Register Token data. It must be
//     greater than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_REGISTER_TOKEN_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The Register Token was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetRegisterToken(const char *data,
                                                        size_t data_size);

// """Retrieves the EVP Hub URL from the parameter storage manager.
// This function retrieves the EVP Hub URL data from the parameter storage
// manager. The function fills the provided buffer with the EVP Hub URL data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the EVP Hub URL data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the EVP Hub URL data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved EVP Hub URL data.
// Returns:
//   kEsfSystemManagerResultOk: The EVP Hub URL was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetEvpHubUrl(char *data,
                                                    size_t *data_size);

// """Sets the EVP Hub URL in the parameter storage manager.
// This function sets the EVP Hub URL in the parameter storage manager.
// - The provided EVP Hub URL data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
// Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     EVP Hub URL. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the EVP Hub URL data. It must be
//     greater than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The EVP Hub URL was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetEvpHubUrl(const char *data,
                                                    size_t data_size);

// """Retrieves the EVP Hub Port from the parameter storage manager.
// This function retrieves the EVP Hub Port data from the parameter storage
// manager. The function fills the provided buffer with the EVP Hub Port data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the EVP Hub Port data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the EVP Hub Port data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved EVP Hub Port data.
// Returns:
//   kEsfSystemManagerResultOk: The EVP Hub Port was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetEvpHubPort(char *data,
                                                     size_t *data_size);

// """Sets the EVP Hub Port in the parameter storage manager.
// This function sets the EVP Hub Port in the parameter storage manager.
// - The provided EVP Hub Port data is stored if it meets certain conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
// Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     EVP Hub Port. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the EVP Hub Port data. It must be
//     greater than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_EVP_HUB_URL_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The EVP Hub Port was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetEvpHubPort(const char *data,
                                                     size_t data_size);

// """Retrieves the EVP IoT Platform from the parameter storage manager.
// This function retrieves the EVP IoT Platform data from the parameter storage
// manager. The function fills the provided buffer with the EVP IoT Platform
// data and updates the size parameter to reflect the size of the retrieved
// data. Args:
//   [OUT] data (char *): Pointer to a buffer where the EVP IoT Platform data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the EVP IoT Platform data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved EVP IoT Platform data.
// Returns:
//   kEsfSystemManagerResultOk: The EVP IoT Platform was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
// """
EsfSystemManagerResult EsfSystemManagerGetEvpIotPlatform(char *data,
                                                         size_t *data_size);

// """Sets the EVP IoT Platform in the parameter storage manager.
// This function sets the EVP IoT Platform in the parameter storage manager.
// - The provided EVP IoT Platform data is stored if it meets certain
// conditions:
// - The data pointer is non-null.
// - The data size is greater than zero and does not exceed the maximum allowed
// size.
// - The data is null-terminated within the provided size.
//  Args:
//   [IN] data (const char *): Pointer to a character array containing the
//     EVP IoT Platform. The pointer must be non-null.
//   [IN] data_size (size_t): Size of the EVP IoT Platform data. It must be
//   greater
//     than zero and less than or equal to
//     `ESF_SYSTEM_MANAGER_PROJECT_ID_MAX_SIZE`.
// Returns:
//   kEsfSystemManagerResultOk: The EVP IoT Platform was successfully set.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
// """
EsfSystemManagerResult EsfSystemManagerSetEvpIotPlatform(const char *data,
                                                         size_t data_size);

// """Retrieves the EVP TLS value from the parameter storage manager.
// This function retrieves the EVP TLS value from the parameter storage manager.
// Based on the stored information, it updates the provided data pointer with
// the corresponding EVP TLS value.
// Args:
//   [OUT] data (EsfSystemManagerEvpTlsValue *): Pointer to an
//     EsfSystemManagerEvpTlsValue enumeration where the EVP TLS value will be
//     stored. The pointer must be non-null.
// Returns:
//   kEsfSystemManagerResultOk: The EVP TLS value was successfully retrieved.
//   kEsfSystemManagerResultParamError: The provided data pointer is null.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The retrieved EVP TLS value
//     is out of the expected range.
// """
EsfSystemManagerResult EsfSystemManagerGetEvpTls(
    EsfSystemManagerEvpTlsValue *data);

// """Sets the EVP TLS value in the parameter storage manager.
// This function sets the EVP TLS value in the parameter storage manager.
// The function updates the storage with the provided EVP TLS value.
// Args:
//   [IN] data (EsfSystemManagerEvpTlsValue): The EVP TLS value to be set. It
//     must be one of the values from the EsfSystemManagerEvpTlsValue
//     enumeration.
// Returns:
//   kEsfSystemManagerResultOk: The EVP TLS value was successfully set.
//   kEsfSystemManagerResultParamError: An invalid value was provided for the
//     EVP TLS.
//   kEsfSystemManagerResultInternalError: An internal error occurred
//     during the operation.
// """
EsfSystemManagerResult EsfSystemManagerSetEvpTls(
    EsfSystemManagerEvpTlsValue data);

// """Retrieves the Root CA from the parameter storage manager.
// This function retrieves the Root CA data from the parameter storage
// manager. The function fills the provided buffer with the Root CA data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Root CA data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Root CA data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Root CA data.
// Returns:
//   kEsfSystemManagerResultOk: The Root CA was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
//   kEsfSystemManagerResultEmptyData: Root CA data is empty.
// """
EsfSystemManagerResult EsfSystemManagerGetRootCa(char *data, size_t *data_size);

// """Retrieves the Root CA Hash from the parameter storage manager.
// This function retrieves the Root CA Hash data from the parameter storage
// manager. The function fills the provided buffer with the Root CA Hash data
// and updates the size parameter to reflect the size of the retrieved data.
// Args:
//   [OUT] data (char *): Pointer to a buffer where the Root CA Hash data
//     will be stored. The buffer must be allocated by the caller and should
//     have enough space for the Root CA Hash data.
//   [IN/OUT] data_size (size_t *): Pointer to a size_t variable. On input, it
//     should specify the size of the provided buffer. On output, it will be
//     updated with the actual size of the retrieved Root CA Hash data.
// Returns:
//   kEsfSystemManagerResultOk: The Root CA Hash was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer or zero size).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The provided buffer is too
//     small to store the data.
//   kEsfSystemManagerResultEmptyData: Root CA Hash data is empty.
// """
EsfSystemManagerResult EsfSystemManagerGetRootCaHash(char *data,
                                                     size_t *data_size);

// """Retrieves the QR mode timeout value from the parameter storage manager.
// This function retrieves the QR mode timeout value from the parameter storage
// manager. The retrieved timeout value is stored in the provided data pointer.
// Args:
//   [OUT] data (int32_t *): Pointer to an integer where the timeout value will
//     be stored. The pointer must be non-null.
// Returns:
//   kEsfSystemManagerResultOk: The QR mode timeout value was successfully
//     retrieved.
//   kEsfSystemManagerResultParamError: The provided data pointer is null.
//   kEsfSystemManagerResultInternalError: An internal error occurred
//     during the operation.
// """
EsfSystemManagerResult EsfSystemManagerGetQrModeTimeoutValue(int32_t *data);

// """Sets the QR mode timeout value in the parameter storage manager.
// This function sets the QR mode timeout value in the parameter storage
// manager. The function updates the storage with the provided QR mode timeout
// value.
// Args:
//   [IN] data (int32_t): The QR mode timeout value to be set. This value is
//     stored in the parameter storage manager and used in subsequent operations
//     that require the QR mode timeout.
// Returns:
//   kEsfSystemManagerResultOk: The QR mode timeout value was successfully set.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as opening/closing the parameter storage manager.
// """
EsfSystemManagerResult EsfSystemManagerSetQrModeTimeoutValue(int32_t data);

// """Retrieves the HW Info from the parameter storage manager.
// This function retrieves the HW Info data from the parameter storage
// manager. The function fills the provided structure with the HW Info data.
// Args:
//   [OUT] data (EsfSystemManagerHwInfo *): Pointer to an EsfSystemManagerHwInfo
//     structure where the HW Info data will be stored. The pointer must be
//     non-null.
// Returns:
//   kEsfSystemManagerResultOk: The HW Info was successfully retrieved.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided (NULL
//     pointer).
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation.
//   kEsfSystemManagerResultEmptyData: HW Info data is empty.
// """
EsfSystemManagerResult EsfSystemManagerGetHwInfo(EsfSystemManagerHwInfo *data);

// """Retrieves the Initial Setting Flag from the parameter storage manager.
// This function retrieves the Initial Setting Flag from the parameter storage
// manager. Based on the stored information, it updates the provided data
// pointer with the corresponding Initial Setting Flag.
// Args:
//   [OUT] data (EsfSystemManagerEvpTlsValue *): Pointer to an
//     EsfSystemManagerEvpTlsValue enumeration where the Initial Setting Flag
//     will be stored. The pointer must be non-null.
// Returns:
//   kEsfSystemManagerResultOk: The Initial Setting Flag was successfully
//   retrieved. kEsfSystemManagerResultParamError: The provided data pointer is
//   null. kEsfSystemManagerResultInternalError: An internal error occurred
//   during the
//     operation.
//   kEsfSystemManagerResultOutOfRange: The retrieved Initial Setting Flag
//     is out of the expected range.
// """
EsfSystemManagerResult EsfSystemManagerGetInitialSettingFlag(
    EsfSystemManagerInitialSettingFlag *data);

// """Sets the Initial Setting Flag in the parameter storage manager.
// This function sets the Initial Setting Flag in the parameter storage manager.
// The function updates the storage with the provided Initial Setting Flag.
// Args:
//   [IN] data (EsfSystemManagerEvpTlsValue): The Initial Setting Flag to be
//   set. It
//     must be one of the values from the EsfSystemManagerEvpTlsValue
//     enumeration.
// Returns:
//   kEsfSystemManagerResultOk: The Initial Setting Flag was successfully set.
//   kEsfSystemManagerResultParamError: An invalid value was provided for the
//     EVP TLS.
//   kEsfSystemManagerResultInternalError: An internal error occurred
//     during the operation.
// """
EsfSystemManagerResult EsfSystemManagerSetInitialSettingFlag(
    EsfSystemManagerInitialSettingFlag data);

// """Retrieves the EVP reset cause from the parameter storage manager.
// This function retrieves the EVP reset cause stored in the parameter storage
// manager and converts it to the EsfSystemManagerEvpResetCause enum format.
// Args:
//   [OUT] evp_reset_cause (EsfSystemManagerEvpResetCause *): Pointer to a
//     variable where the EVP reset cause will be stored. The pointer must be
//     non-null.
// Returns:
//   kEsfSystemManagerResultOk: The EVP reset cause was successfully retrieved.
//   kEsfSystemManagerResultParamError: The provided evp_reset_cause pointer is
//     null.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as opening/closing the parameter storage manager or
//     retrieving the reset cause.
// """
EsfSystemManagerResult EsfSystemManagerGetEvpResetCause(
    EsfSystemManagerEvpResetCause *evp_reset_cause);

// """Sets the EVP reset cause in the parameter storage manager.
// This function saves the provided EVP reset cause to the parameter storage
// manager. If the reset cause is set to disable, the stored value is cleared.
// Args:
//   [IN] evp_reset_cause (EsfSystemManagerEvpResetCause): The EVP reset cause
//     to be set. Must be a valid value within the EsfSystemManagerEvpResetCause
//     enum.
// Returns:
//   kEsfSystemManagerResultOk: The EVP reset cause was successfully set.
//   kEsfSystemManagerResultParamError: The provided EVP reset cause is invalid.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as opening/closing the parameter storage manager or
//     saving the reset cause.
// """
EsfSystemManagerResult EsfSystemManagerSetEvpResetCause(
    EsfSystemManagerEvpResetCause evp_reset_cause);

// """Retrieves the reset cause from the parameter storage manager.
// This function retrieves the reset cause stored in the parameter storage
// manager and converts it to the EsfSystemManagerResetCause enum format.
// Args:
//   [OUT] reset_cause (EsfSystemManagerResetCause *): Pointer to a variable
//     where the reset cause will be stored. The pointer must be non-null.
// Returns:
//   kEsfSystemManagerResultOk: The reset cause was successfully retrieved.
//   kEsfSystemManagerResultParamError: The provided reset_cause pointer is
//   null. kEsfSystemManagerResultInternalError: An internal error occurred
//   during the
//     operation, such as opening/closing the parameter storage manager or
//     retrieving the reset cause.
// """
EsfSystemManagerResult EsfSystemManagerGetResetCause(
    EsfSystemManagerResetCause *reset_cause);

// """Sets the reset cause in the parameter storage manager.
// This function saves the provided reset cause to the parameter storage
// manager. The reset cause is converted to a string format before being stored.
// Args:
//   [IN] reset_cause (EsfSystemManagerResetCause): The reset cause to be set.
//     Must be a valid value within the EsfSystemManagerResetCause enum.
// Returns:
//   kEsfSystemManagerResultOk: The reset cause was successfully set.
//   kEsfSystemManagerResultParamError: The provided reset cause is invalid.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as opening/closing the parameter storage manager or
//     saving the reset cause.
// """
EsfSystemManagerResult EsfSystemManagerSetResetCause(
    EsfSystemManagerResetCause reset_cause);

// """Sets the exception information in the parameter storage manager.
// This function saves the exception information, including reset causes and
// additional exception details, to the parameter storage manager. If the
// exception information already exists, the function does nothing.
// Args:
//   None
// Returns:
//   kEsfSystemManagerResultOk: The exception information was successfully set
//     or already exists.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as retrieving or saving the exception information.
// """
EsfSystemManagerResult EsfSystemManagerSetExceptionInfo(void);

// """Sends the reset cause to the logging system.
// This function retrieves the reset cause from the parameter storage manager,
// determines its type (e.g., EVP or general reset cause), and sends it to the
// logging system. After successfully sending the reset cause, it clears the
// stored reset cause from the parameter storage manager.
// Args:
//   None
// Returns:
//   kEsfSystemManagerResultOk: The reset cause was successfully sent.
//   kEsfSystemManagerResultParamError: Invalid parameters were provided.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as retrieving, sending, or clearing the reset cause.
// """
EsfSystemManagerResult EsfSystemManagerSendResetCause(void);

// """Uploads the ExceptionInfo to the logging system.
// This function retrieves the ExceptionInfo from the parameter storage
// manager, processes it, and sends it to the logging system. After successful
// upload, the ExceptionInfo is cleared from the storage.
// Args:
//   None
// Returns:
//   kEsfSystemManagerResultOk: The ExceptionInfo was successfully uploaded.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as retrieving, processing, or clearing the ExceptionInfo.
// """
EsfSystemManagerResult EsfSystemManagerUploadExceptionInfo(void);

// """Checks if a reboot is required based on the reset cause.
// This function determines whether a reboot is necessary by analyzing the
// reset cause stored in the parameter storage manager. The result is stored
// in the provided reset_flag pointer.
// Args:
//   [OUT] reset_flag (bool *): Pointer to a boolean variable where the reboot
//     requirement flag will be stored. The pointer must be non-null.
// Returns:
//   kEsfSystemManagerResultOk: The reboot requirement was successfully
//     determined.
//   kEsfSystemManagerResultParamError: The provided reset_flag pointer is null.
//   kEsfSystemManagerResultInternalError: An internal error occurred during the
//     operation, such as retrieving the reset cause or checking the reboot
//     flag.
// """
EsfSystemManagerResult EsfSystemManagerIsNeedReboot(bool *reset_flag);

// """Executes a system reboot with the specified reboot type.
// This function initiates a system reboot with the provided reboot type.
// The reboot type determines the reason for the reboot and affects how the
// system handles the reboot process.
// Args:
//   [IN] reboot_type (EsfSystemManagerRebootType): The type of reboot to be
//     executed. Must be one of the values from the EsfSystemManagerRebootType
//     enumeration.
// Returns:
//   None (void): This function does not return as it performs a system reboot.
// """
void EsfSystemManagerExecReboot(EsfSystemManagerRebootType reboot_type);

// """Performs migration of legacy data to the parameter storage manager.
// This function migrates various types of legacy data (RootAuth, DeviceManifest,
// HwInfo, and EVP Setup Info) from the old format to the new parameter storage
// manager format. The migration is performed sequentially for each data type.
// 
// Returns:
//   kEsfSystemManagerResultOk: All migration data was successfully processed.
//   kEsfSystemManagerResultInternalError: An internal error occurred during
//     migration.
//   kEsfSystemManagerResultEmptyData: Some migration data was empty (treated
//     as success).
// """
EsfSystemManagerResult EsfSystemManagerMigration(void);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_INCLUDE_SYSTEM_MANAGER_H_
