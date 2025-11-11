/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_COMMON_H_
#define ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// """Gets the size [Byte] of a member of the structure.

// Obtains the size of a member of a structure with the sizeof operator.

// Args:
//     [IN] type : The type name of the structure.
//     [IN] member : The member names of the structure.

// Examples:
//     ParameterStorageManagerStructure structure;
//     ESF_PARAMETER_STORAGE_MANAGER_MEMBER_SIZEOF(structure, member);

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_MEMBER_SIZEOF(type, member) \
  (sizeof(((type*)NULL)->member))

// """Gets the size [Byte] of the array type of the binary data of the members
//    of the data structure.

// Get the size of the array type of binary data with the sizeof operator.

// Args:
//     [IN] type : The type name of the structure.
//     [IN] member : The member names of the array type of binary data.

// Examples:
//     ParameterStorageManagerStructure structure;
//     ESF_PARAMETER_STORAGE_MANAGER_BINARY_ARRAY_SIZEOF(structure, member);

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_BINARY_ARRAY_SIZEOF(type, member) \
  ESF_PARAMETER_STORAGE_MANAGER_MEMBER_SIZEOF(type, member.data)

// """Gets the size [Byte] of the array type of the offset specified binary data
// of a member of the data structure.

// Get the size of the array type of the offset specified binary data with the
// sizeof operator.

// Args:
//     [IN] type : The type name of the structure.
//     [IN] member : Member names of the array type for offset-specified binary
//                  data.

// Examples:
//     ParameterStorageManagerStructure structure;
//     ESF_PARAMETER_STORAGE_MANAGER_OFFSET_BINARY_ARRAY_SIZEOF(structure,
//     member);

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_OFFSET_BINARY_ARRAY_SIZEOF(type, member) \
  ESF_PARAMETER_STORAGE_MANAGER_MEMBER_SIZEOF(type, member.data)

// """Gets the size [Byte] of the string type of the members of the data
// structure.

// Get the size of a string type with the sizeof operator.

// Args:
//     [IN] type : The type name of the structure.
//     [IN] member : Member name of the string type.

// Examples:
//     ParameterStorageManagerStructure structure;
//     ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(structure, member);

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(type, member) \
  ESF_PARAMETER_STORAGE_MANAGER_MEMBER_SIZEOF(type, member)

// """Gets the size [Byte] of the raw data type of a member of the data
// structure.

// Get the size of the raw data type with the sizeof operator.

// Args:
//     [IN] type : The type name of the member to be treated as a raw data type.
//     [IN] member : The name of the last member of the raw data type.

// Examples:
//     ParameterStorageManagerStructure structure;
//     ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(structure, member);

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(type, tail_member) \
  offsetof(type, tail_member) +                                     \
      ESF_PARAMETER_STORAGE_MANAGER_MEMBER_SIZEOF(type, tail_member)

// """Determines if a particular member of the mask structure is valid.

// Determines whether a particular member of the mask structure is valid. If the
// mask structure is valid, the value of the member specified by member is
// returned in bool type.

// Args:
//     [IN] type : The type name of the structure.
//     [IN] member : Member name of the raw data type.
//     [IN] obj (EsfParameterStorageManagerMask): Mask structure.

// Examples:
//     ParameterStorageManagerStructure structure;
//     EsfParameterStorageManagerMask obj;
//     ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(structure, member, obj);

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(type, member, obj) \
  ((obj) != ESF_PARAMETER_STORAGE_MANAGER_INVALID_MASK) &&               \
      (((const type*)(obj))->member)

// """Determines if a particular member of the structure is empty.

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] obj (const EsfParameterStorageManagerBinaryArray*): The member
//              of binary array type.

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_BINARY_ARRAY_IS_EMPTY(obj) \
  ((obj) != NULL && (obj)->size == 0)

// """Determines if a particular member of the structure is empty.

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] obj (const EsfParameterStorageManagerBinary*): The member
//              of binary pointer type.

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_BINARY_POINTER_IS_EMPTY(obj) \
  ((obj) != NULL && (obj)->data != NULL && (obj)->size == 0)

// """Determines if a particular member of the structure is empty.

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] obj (const EsfParameterStorageManagerOffsetBinaryArray*): The member
//              of offset binary array type.

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_OFFSET_BINARY_ARRAY_IS_EMPTY(obj) \
  ((obj) != NULL && (obj)->offset == 0 && (obj)->size == 0)

// """Determines if a particular member of the structure is empty.

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] obj (const EsfParameterStorageManagerOffsetBinary*): The member of
//              offset binary pointer type.

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_OFFSET_BINARY_POINTER_IS_EMPTY(obj) \
  ((obj) != NULL && (obj)->data != NULL && (obj)->offset == 0 &&          \
   (obj)->size == 0)

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] obj (const char*): The member of string type.

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_STRING_IS_EMPTY(obj) \
  ((obj) != NULL && (obj)[0] == '\0')

// Determines if a particular member of the structure is empty.
// If a particular member of a struct is empty, the return value is true,
// otherwise it returns false.

// Args:
//     [IN] obj (const EsfParameterStorageManagerRaw*): The member of raw type.

// Note:
// """
#define ESF_PARAMETER_STORAGE_MANAGER_RAW_IS_EMPTY(obj) \
  ((obj) != NULL && (obj)->size == 0)

// This code defines an enumeration type for the result of executing an API.
typedef enum EsfParameterStorageManagerStatus {
  kEsfParameterStorageManagerStatusOk,                  // No errors
  kEsfParameterStorageManagerStatusInvalidArgument,     // The argument is
                                                        // invalid
  kEsfParameterStorageManagerStatusFailedPrecondition,  // Status error
  kEsfParameterStorageManagerStatusNotFound,           // The resource not found
  kEsfParameterStorageManagerStatusOutOfRange,         // Invalid range was
                                                       // specified
  kEsfParameterStorageManagerStatusPermissionDenied,   // Operation not
                                                       // allowed
  kEsfParameterStorageManagerStatusResourceExhausted,  // Out of resources
  kEsfParameterStorageManagerStatusDataLoss,           // Data lost
  kEsfParameterStorageManagerStatusUnavailable,  // Access error to storage
                                                 // area
  kEsfParameterStorageManagerStatusInternal,     // Internal error
  kEsfParameterStorageManagerStatusTimedOut,     // Could not be completed the
                                                 // operation
  kEsfParameterStorageManagerStatusMax,  // The number of definitions for
                                         // EsfParameterStorageManagerStatus.
} EsfParameterStorageManagerStatus;

// An enumeration that defines the state of the temporary data store when an
// update begins.
typedef enum EsfParameterStorageManagerUpdateType {
  // There is no data saved.
  kEsfParameterStorageManagerUpdateEmpty,

  // The same data is stored as in the actual data storage area.
  kEsfParameterStorageManagerUpdateCopy,

  // The maximum number.
  kEsfParameterStorageManagerUpdateTypeMax,
} EsfParameterStorageManagerUpdateType;

// An enumeration type that defines the data supported by Parameter Storage
// Manager.
typedef enum EsfParameterStorageManagerItemID {
  // Enrollment
  kEsfParameterStorageManagerItemDpsURL,         // DPS URL
  kEsfParameterStorageManagerItemCommonName,     // Common Name
  kEsfParameterStorageManagerItemDpsScopeID,     // DPS Scope ID
  kEsfParameterStorageManagerItemProjectID,      // Project ID
  kEsfParameterStorageManagerItemRegisterToken,  // Register Token

  // Network
  kEsfParameterStorageManagerItemWiFiSSID,        // Wi-Fi SSID
  kEsfParameterStorageManagerItemWiFiPassword,    // Wi-Fi Password
  kEsfParameterStorageManagerItemWiFiEncryption,  // Wi-Fi Encryption
  kEsfParameterStorageManagerItemIPAddress,       // IP Address
  kEsfParameterStorageManagerItemSubnetMask,      // Subnet Mask
  kEsfParameterStorageManagerItemGateway,         // Gateway
  kEsfParameterStorageManagerItemDNS,             // DNS
  kEsfParameterStorageManagerItemIPMethod,        // DHCP enable/disable setting
  kEsfParameterStorageManagerItemNetIfKind,       // Wi-Fi/Ethernet designation.
  kEsfParameterStorageManagerItemIPv6IPAddress,   // IPv6 IP Address
  kEsfParameterStorageManagerItemIPv6SubnetMask,  // IPv6 Subnet Mask
  kEsfParameterStorageManagerItemIPv6Gateway,     // IPv6 Gateway
  kEsfParameterStorageManagerItemIPv6DNS,         // IPv6 DNS
  kEsfParameterStorageManagerItemWiFiApSSID,      // SSID for Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApPassword,  // Password for Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApEncryption,  // Encryption settings for
                                                    // Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApChannel,  // Channel setting for Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApIPAddress,   // IP address of Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApSubnetMask,  // Subnet Mask of Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApGateway,     // Gateway of Wi-Fi AP
  kEsfParameterStorageManagerItemWiFiApDNS,         // DNS of Wi-Fi AP
  kEsfParameterStorageManagerItemProxyURL,       // The URL of the Proxy server
  kEsfParameterStorageManagerItemProxyPort,      // The port number of the Proxy
                                                 // server
  kEsfParameterStorageManagerItemProxyUserName,  // Proxy server user name
  kEsfParameterStorageManagerItemProxyPassword,  // Proxy server password

  // EVP
  kEsfParameterStorageManagerItemEvpHubURL,       // The URL of the EVP Hub
  kEsfParameterStorageManagerItemEvpHubPort,      // Port number of the EVP Hub
  kEsfParameterStorageManagerItemEvpIotPlatform,  // The IoT Platform of the EVP
                                                  // Runtime
  kEsfParameterStorageManagerItemPkiRootCerts,    // Root CA
  kEsfParameterStorageManagerItemPkiRootCertsHash,  // Root CA Hash
  kEsfParameterStorageManagerItemEvpTls,  // TLS authentication of EVP is
                                          // required.
  kEsfParameterStorageManagerItemDeviceManifest,  // Device Manifest.

  // Log collecting
  kEsfParameterStorageManagerItemDebugLogLevel,  // Debug log level
  kEsfParameterStorageManagerItemEventLogLevel,  // The level of the event log
  // The output destination of the debug log
  kEsfParameterStorageManagerItemDebugLogDestination,
  kEsfParameterStorageManagerItemLogFilter,                // Log Filter
  kEsfParameterStorageManagerItemLogUseFlash,              // Use Flash
  kEsfParameterStorageManagerItemStorageName,              // Storage Name
  kEsfParameterStorageManagerItemStorageSubDirectoryPath,  // Storage
                                                           // SubDirectory Path

  // Clock Manager
  kEsfParameterStorageManagerItemNTPServer,  // The URL of the NTP server.
  // The synchronization cycle of the NTP client.
  kEsfParameterStorageManagerItemNTPSyncInterval,
  // NTP client monitoring cycle.
  kEsfParameterStorageManagerItemNTPPollingTime,
  kEsfParameterStorageManagerItemSkipModeSettings,  // Skip Mode Settings.
  kEsfParameterStorageManagerItemLimitPacketTime,   // Limit Packet Time
  kEsfParameterStorageManagerItemLimitRTCCorrectionValue,  // Limit RTC
                                                           // Correction Value
  kEsfParameterStorageManagerItemSanityLimit,              // Sanity Limit
  kEsfParameterStorageManagerItemSlewModeSettings,         // Slew Mode Settings
  kEsfParameterStorageManagerItemStableRTCCorrectionValue,  // Stable RTC
                                                            // Correction Value
  kEsfParameterStorageManagerItemStableSyncNumber,  // Stable Sync Number

  // Power Manager
  kEsfParameterStorageManagerItemFactoryResetFlag,  // Factory Reset flag.
  // Abnormality detection of intermittent operation.
  kEsfParameterStorageManagerItemRTCErrorDetection,
  // PQA parameter for intermittent operation.
  kEsfParameterStorageManagerItemRTCPQAParameter,
  kEsfParameterStorageManagerItemBatteryInformation,  // Battery information.
  // Network information for intermittent operation.
  kEsfParameterStorageManagerItemRTCNetworkInformation,
  // Config for intermittent operation
  kEsfParameterStorageManagerItemRTCConfig,
  kEsfParameterStorageManagerItemHoursMeter,  // Hours Meter

  // SAS
  kEsfParameterStorageManagerItemSAS,  // Encrypted data of SAS
                                       // URL/Cert/Directory Path

  // SystemApp
  kEsfParameterStorageManagerItemQRModeStateFlag,     // QR Mode state flag
  kEsfParameterStorageManagerItemInitialSettingFlag,  // Initial setting flag
  kEsfParameterStorageManagerItemHWInfoText,          // HW info text

  // Version
  kEsfParameterStorageManagerItemMCULoaderVersion,     // MCU Loader version.
  kEsfParameterStorageManagerItemMCUFWVersion,         // MCU FW Version.
  kEsfParameterStorageManagerItemLMTCustomVersion,     // LMT Custom version
  kEsfParameterStorageManagerItemPreWBCustomVersion,   // PreWB Custom version
  kEsfParameterStorageManagerItemGAMMACustomVersion,   // GAMMA Custom version
  kEsfParameterStorageManagerItemLSCCustomVersion,     // LSC Custom version
  kEsfParameterStorageManagerItemLSCRawCustomVersion,  // LSC RAW Custom version
  kEsfParameterStorageManagerItemDEWARPCustomVersion,  // DEWARP Custom version
  kEsfParameterStorageManagerItemSensorLoaderVersion,  // SensorLoader version.
  kEsfParameterStorageManagerItemSensorFWVersion,      // SensorFW version.
  kEsfParameterStorageManagerItemAIModelVersionSlot1,  // Version for slot 0 of
                                                       // AI Model.
  kEsfParameterStorageManagerItemAIModelVersionSlot2,  // Version for slot 1 of
                                                       // AI Model.
  kEsfParameterStorageManagerItemAIModelVersionSlot3,  // Version for slot 2 of
                                                       // AI Model.
  kEsfParameterStorageManagerItemAIModelVersionSlot4,  // Version for slot 3 of
                                                       // AI Model.

  // LastUpdate
  kEsfParameterStorageManagerItemMCULoaderLastUpdate,  // MCU Loader last update
                                                       // time.
  kEsfParameterStorageManagerItemMCUFWLastUpdate,  // MCU FW last update time.
  kEsfParameterStorageManagerItemLMTCustomLastUpdate,  // LMT Custom last update
                                                       // time.
  kEsfParameterStorageManagerItemPreWBCustomLastUpdate,  // PreWB Custom last
                                                         // update time.
  kEsfParameterStorageManagerItemGAMMACustomLastUpdate,  // GAMMA Custom last
                                                         // update time.
  kEsfParameterStorageManagerItemLSCCustomLastUpdate,  // LSC Custom last update
                                                       // time.
  kEsfParameterStorageManagerItemLSCRawCustomLastUpdate,  // LSC RAW Custom last
                                                          // update time.
  kEsfParameterStorageManagerItemDEWARPCustomLastUpdate,  // DEWARP Custom last
                                                          // update time.
  // The last update time of SensorLoader.
  kEsfParameterStorageManagerItemSensorLoaderLastUpdate,
  kEsfParameterStorageManagerItemSensorFWLastUpdate,      // Last update time of
                                                          // SensorFW.
  kEsfParameterStorageManagerItemAIModelLastUpdateSlot1,  // Last update time of
                                                          // slot 0 of AI Model.
  kEsfParameterStorageManagerItemAIModelLastUpdateSlot2,  // Last update time of
                                                          // slot 1 of AI Model.
  kEsfParameterStorageManagerItemAIModelLastUpdateSlot3,  // Last update time of
                                                          // slot 2 of AI Model.
  kEsfParameterStorageManagerItemAIModelLastUpdateSlot4,  // Last update time of
                                                          // slot 3 of AI Model.

  // Flash Address
  kEsfParameterStorageManagerItemSensorLoaderFlashAddress,  // SensorLoader
                                                            // Flash Address
  kEsfParameterStorageManagerItemSensorFWFlashAddress,      // SensorFW Flash
                                                            // Address

  // AI Model Parameter
  // Parameters for slot 0 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterSlot0,
  // Parameters for slot 1 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterSlot1,
  // Parameters for slot 2 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterSlot2,
  // Parameters for slot 3 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterSlot3,
  // Parameters for slot 4 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterSlot4,
  // Hash value of slot 0 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterHashSlot0,
  // Hash value of slot 1 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterHashSlot1,
  // Hash value of slot 2 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterHashSlot2,
  // Hash value of slot 3 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterHashSlot3,
  // Hash value of slot 4 of AI Model.
  kEsfParameterStorageManagerItemAIModelParameterHashSlot4,

// IMX500 CameraSetup
#if 0  // Parameter Storage Manager will not handle fa_data3,
       // please check senscord
  kEsfParameterStorageManagerItemLMTStd,        // LMT (STD surface).
  kEsfParameterStorageManagerItemPreWBStd,      // PreWB (STD surface).
  kEsfParameterStorageManagerItemGAMMAStd,      // GAMMA (STD surface).
  kEsfParameterStorageManagerItemLSCStd,        // LSC (STD surface)
  kEsfParameterStorageManagerItemLSCRawStd,     // LSC RAW (STD surface)
  kEsfParameterStorageManagerItemDEWARPStd,     // DEWARP (STD surface)
#endif
  kEsfParameterStorageManagerItemLMTCustom,     // LMT (Custom surface)
  kEsfParameterStorageManagerItemPreWBCustom,   // PreWB (Custom surface)
  kEsfParameterStorageManagerItemGAMMACustom,   // GAMMA (Custom surface)
  kEsfParameterStorageManagerItemLSCCustom,     // LSC (Custom surface)
  kEsfParameterStorageManagerItemLSCRawCustom,  // LSC RAW (Custom surface)
  kEsfParameterStorageManagerItemDEWARPCustom,  // DEWARP (Custom surface)
  kEsfParameterStorageManagerItemLMTHashStd,  // Hash value of LMT (STD surface)
  // Hash value of PreWB (STD surface)
  kEsfParameterStorageManagerItemPreWBHashStd,
  kEsfParameterStorageManagerItemGAMMAHashStd,  // Hash value of GAMMA (STD
                                                // side)
  kEsfParameterStorageManagerItemLSCHashStd,    // Hash value of LSC (STD side).
  kEsfParameterStorageManagerItemLSCRawHashStd,  // Hash value of LSC RAW (STD
                                                 // side).
  kEsfParameterStorageManagerItemDEWARPHashStd,  // Hash value of DEWARP (STD
                                                 // side)
  kEsfParameterStorageManagerItemLMTHashCustom,  // Hash value of LMT (Custom
                                                 // side)
  // Hash value of PreWB (Custom side)
  kEsfParameterStorageManagerItemPreWBHashCustom,
  // Hash value of GAMMA (Custom side)
  kEsfParameterStorageManagerItemGAMMAHashCustom,
  kEsfParameterStorageManagerItemLSCHashCustom,     // Hash value of LSC (Custom
                                                    // side)
  kEsfParameterStorageManagerItemLSCRawHashCustom,  // Hash value of LSC RAW
                                                    // (Custom side)
  // Hash value of DEWARP (Custom side)
  kEsfParameterStorageManagerItemDEWARPHashCustom,
  // Mode of the image quality adjustment value.
  kEsfParameterStorageManagerItemCalibrationParamMode,

  // Additional data
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot0,
  kEsfParameterStorageManagerItemAIISPAIModelVersionSlot1,
  kEsfParameterStorageManagerItemAIISPAIModelVersionSlot2,
  kEsfParameterStorageManagerItemAIISPAIModelVersionSlot3,
  kEsfParameterStorageManagerItemAIISPAIModelVersionSlot4,
  kEsfParameterStorageManagerItemAIISPAIModelLastUpdateSlot1,
  kEsfParameterStorageManagerItemAIISPAIModelLastUpdateSlot2,
  kEsfParameterStorageManagerItemAIISPAIModelLastUpdateSlot3,
  kEsfParameterStorageManagerItemAIISPAIModelLastUpdateSlot4,
  kEsfParameterStorageManagerItemAIISPLoaderFlashAddress,
  kEsfParameterStorageManagerItemAIISPFWFlashAddress,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot1,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot2,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot3,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot4,
  kEsfParameterStorageManagerItemAIModelSlotInfo,
  kEsfParameterStorageManagerItemAIISPAIModelSlotInfo,
  kEsfParameterStorageManagerItemEsfSensorConfig,
  kEsfParameterStorageManagerItemDNSSecondary,
  kEsfParameterStorageManagerItemNTPServerSecondary,

  // fixed
  kEsfParameterStorageManagerItemCustom,  // Identifier for custom operation.
  kEsfParameterStorageManagerItemMax,     // The maximum number of data IDs.
} EsfParameterStorageManagerItemID;

// An enumerated type that defines the data type of a data member.
typedef enum EsfParameterStorageManagerItemType {
  kEsfParameterStorageManagerItemTypeBinaryArray,  // Array type of binary data.
  kEsfParameterStorageManagerItemTypeBinaryPointer,  // Pointer type of binary
                                                     // data.
  // The array type of the offset specified binary data.
  kEsfParameterStorageManagerItemTypeOffsetBinaryArray,
  // The pointer type of the offset specified binary data
  kEsfParameterStorageManagerItemTypeOffsetBinaryPointer,
  kEsfParameterStorageManagerItemTypeString,  // String type
  kEsfParameterStorageManagerItemTypeRaw,     // Raw data type
  kEsfParameterStorageManagerItemTypeMax,  // The maximum number of data type.
} EsfParameterStorageManagerItemType;

// Structure that sets a pointer to the buffer that stores binary data.
typedef struct EsfParameterStorageManagerBinary {
  // The buffer size, or data size.
  // Before calling the Load function, this is the buffer size. After calling
  // it, this is the data size.
  // When calling the Save function, this is the data size.
  uint32_t size;

  // Buffer where data is stored. NULL cannot be set.
  uint8_t* data;
} EsfParameterStorageManagerBinary;

// Structure that sets a pointer to the buffer that stores binary data. Can also
// specify the offset, which is the data position in the data storage area.
typedef struct EsfParameterStorageManagerOffsetBinary {
  // Data position in the data storage area.
  uint32_t offset;

  // The buffer size, or data size.
  // Before calling the Load function, this is the buffer size. After calling
  // it, this is the data size.
  // When calling the Save function, this is the data size.
  uint32_t size;

  // Buffer where data is stored. NULL cannot be set.
  uint8_t* data;
} EsfParameterStorageManagerOffsetBinary;

// A handle for controlling Parameter Storage Manager.
typedef int32_t EsfParameterStorageManagerHandle;
#define ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE \
  ((EsfParameterStorageManagerHandle) - 1)

// Alias for the pointer to the mask structure.
typedef uintptr_t EsfParameterStorageManagerMask;
#define ESF_PARAMETER_STORAGE_MANAGER_INVALID_MASK \
  ((EsfParameterStorageManagerMask)NULL)

// Alias for the pointer to the data structure.
typedef uintptr_t EsfParameterStorageManagerData;
#define ESF_PARAMETER_STORAGE_MANAGER_INVALID_DATA \
  ((EsfParameterStorageManagerData)NULL)

// Alias for the registration identifier of Factory Reset.
typedef int32_t EsfParameterStorageManagerFactoryResetID;
#define ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID \
  ((EsfParameterStorageManagerFactoryResetID) - 1)

// A pointer type to a function that performs a Save operation on the data
// structure.
typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerItemAccessorSave)(const void* item,
                                                 void* private_data);

// Pointer type to a function that performs a Load operation on the data
// structure.
typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerItemAccessorLoad)(void* item,
                                                 void* private_data);

// Pointer type of the function that executes Clear operation on the data
// structure.
typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerItemAccessorClear)(void* private_data);

// Pointer type of a function that cancels a Save or Clear operation on a data
// structure.
typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerItemAccessorCancel)(void* private_data);

// Pointer type of the function that judges whether the member of the mask
// structure is valid or not.
typedef bool (*EsfParameterStorageManagerItemMaskIsEnabled)(
    EsfParameterStorageManagerMask mask);

// Pointer type of the function to register Factory Reset.
typedef EsfParameterStorageManagerStatus (
    *EsfParameterStorageManagerRegisterFactoryResetType)(void* private_data);

// A structure that defines operations on a data structure.
// The members of this structure cannot be set to NULL.
typedef struct EsfParameterStorageManagerItemAccessor {
  EsfParameterStorageManagerItemAccessorSave save;
  EsfParameterStorageManagerItemAccessorLoad load;
  EsfParameterStorageManagerItemAccessorClear clear;
  EsfParameterStorageManagerItemAccessorCancel cancel;
} EsfParameterStorageManagerItemAccessor;

// A structure that sets member information of a data structure.
typedef struct EsfParameterStorageManagerMemberInfo {
  // Data type such as IP address. If you want to use other data, please specify
  // "kEsfParameterStorageManagerItemCustom".
  EsfParameterStorageManagerItemID id;

  // Data type such as a string.
  EsfParameterStorageManagerItemType type;

  // Value obtained using offsetof().
  size_t offset;

  // The maximum size of the data.
  // Use a macro to set the value of this member.
  size_t size;

  // A function pointer that checks whether the mask is valid.
  EsfParameterStorageManagerItemMaskIsEnabled enabled;

  // A pointer to store the function to be used when
  // "kEsfParameterStorageManagerItemCustom" is selected.
  const EsfParameterStorageManagerItemAccessor* custom;
} EsfParameterStorageManagerMemberInfo;

// A structure that sets the information of a data structure.
typedef struct EsfParameterStorageManagerStructInfo {
  // The length of the "items" array.
  size_t items_num;

  // The array that stores member information.
  const EsfParameterStorageManagerMemberInfo* items;
} EsfParameterStorageManagerStructInfo;

#ifdef __cplusplus
}
#endif

#endif  // ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_COMMON_H_
