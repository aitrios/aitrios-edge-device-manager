/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_COMMON_H_  // NOLINT
#define ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_COMMON_H_  // NOLINT

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
  (((obj) != NULL) && ((obj)->data != NULL) && ((obj)->offset == 0) && \
  ((obj)->size == 0))

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
  kEsfParameterStorageManagerItemDpsURL,
  kEsfParameterStorageManagerItemCommonName,
  kEsfParameterStorageManagerItemDpsScopeID,
  kEsfParameterStorageManagerItemProjectID,
  kEsfParameterStorageManagerItemRegisterToken,
  kEsfParameterStorageManagerItemWiFiSSID,
  kEsfParameterStorageManagerItemWiFiPassword,
  kEsfParameterStorageManagerItemWiFiEncryption,
  kEsfParameterStorageManagerItemIPAddress,
  kEsfParameterStorageManagerItemSubnetMask,
  kEsfParameterStorageManagerItemGateway,
  kEsfParameterStorageManagerItemDNS,
  kEsfParameterStorageManagerItemIPMethod,
  kEsfParameterStorageManagerItemNetIfKind,
  kEsfParameterStorageManagerItemIPv6IPAddress,
  kEsfParameterStorageManagerItemIPv6SubnetMask,
  kEsfParameterStorageManagerItemIPv6Gateway,
  kEsfParameterStorageManagerItemIPv6DNS,
  kEsfParameterStorageManagerItemWiFiApSSID,
  kEsfParameterStorageManagerItemWiFiApPassword,
  kEsfParameterStorageManagerItemWiFiApEncryption,
  kEsfParameterStorageManagerItemWiFiApChannel,
  kEsfParameterStorageManagerItemWiFiApIPAddress,
  kEsfParameterStorageManagerItemWiFiApSubnetMask,
  kEsfParameterStorageManagerItemWiFiApGateway,
  kEsfParameterStorageManagerItemWiFiApDNS,
  kEsfParameterStorageManagerItemProxyURL,
  kEsfParameterStorageManagerItemProxyPort,
  kEsfParameterStorageManagerItemProxyUserName,
  kEsfParameterStorageManagerItemProxyPassword,
  kEsfParameterStorageManagerItemEvpHubURL,
  kEsfParameterStorageManagerItemEvpHubPort,
  kEsfParameterStorageManagerItemEvpIotPlatform,
  kEsfParameterStorageManagerItemPkiRootCerts,
  kEsfParameterStorageManagerItemPkiRootCertsHash,
  kEsfParameterStorageManagerItemEvpTls,
  kEsfParameterStorageManagerItemDeviceManifest,
  kEsfParameterStorageManagerItemDebugLogLevel,
  kEsfParameterStorageManagerItemEventLogLevel,
  kEsfParameterStorageManagerItemDebugLogDestination,
  kEsfParameterStorageManagerItemLogFilter,
  kEsfParameterStorageManagerItemLogUseFlash,
  kEsfParameterStorageManagerItemStorageName,
  kEsfParameterStorageManagerItemStorageSubDirectoryPath,
  kEsfParameterStorageManagerItemDebugLogLevel2,
  kEsfParameterStorageManagerItemEventLogLevel2,
  kEsfParameterStorageManagerItemDebugLogDestination2,
  kEsfParameterStorageManagerItemLogFilter2,
  kEsfParameterStorageManagerItemLogUseFlash2,
  kEsfParameterStorageManagerItemStorageName2,
  kEsfParameterStorageManagerItemStorageSubDirectoryPath2,
  kEsfParameterStorageManagerItemDebugLogLevel3,
  kEsfParameterStorageManagerItemEventLogLevel3,
  kEsfParameterStorageManagerItemDebugLogDestination3,
  kEsfParameterStorageManagerItemLogFilter3,
  kEsfParameterStorageManagerItemLogUseFlash3,
  kEsfParameterStorageManagerItemStorageName3,
  kEsfParameterStorageManagerItemStorageSubDirectoryPath3,
  kEsfParameterStorageManagerItemDebugLogLevel4,
  kEsfParameterStorageManagerItemEventLogLevel4,
  kEsfParameterStorageManagerItemDebugLogDestination4,
  kEsfParameterStorageManagerItemLogFilter4,
  kEsfParameterStorageManagerItemLogUseFlash4,
  kEsfParameterStorageManagerItemStorageName4,
  kEsfParameterStorageManagerItemStorageSubDirectoryPath4,
  kEsfParameterStorageManagerItemNTPServer,
  kEsfParameterStorageManagerItemNTPSyncInterval,
  kEsfParameterStorageManagerItemNTPPollingTime,
  kEsfParameterStorageManagerItemSkipModeSettings,
  kEsfParameterStorageManagerItemLimitPacketTime,
  kEsfParameterStorageManagerItemLimitRTCCorrectionValue,
  kEsfParameterStorageManagerItemSanityLimit,
  kEsfParameterStorageManagerItemSlewModeSettings,
  kEsfParameterStorageManagerItemStableRTCCorrectionValue,
  kEsfParameterStorageManagerItemStableSyncNumber,
  kEsfParameterStorageManagerItemSystemError,
  kEsfParameterStorageManagerItemFactoryResetFlag,
  kEsfParameterStorageManagerItemRTCErrorDetection,
  kEsfParameterStorageManagerItemRTCPQAParameter,
  kEsfParameterStorageManagerItemBatteryInformation,
  kEsfParameterStorageManagerItemRTCNetworkInformation,
  kEsfParameterStorageManagerItemRTCConfig,
  kEsfParameterStorageManagerItemHoursMeter,
  kEsfParameterStorageManagerItemSAS,
  kEsfParameterStorageManagerItemQRModeStateFlag,
  kEsfParameterStorageManagerItemInitialSettingFlag,
  kEsfParameterStorageManagerItemHWInfoText,
  kEsfParameterStorageManagerItemMCULoaderVersion,
  kEsfParameterStorageManagerItemSensorLoaderVersion,
  kEsfParameterStorageManagerItemMCUFWLastUpdate,
  kEsfParameterStorageManagerItemSensorLoaderLastUpdate,
  kEsfParameterStorageManagerItemSensorFWLastUpdate,
  kEsfParameterStorageManagerItemSensorAIModelFlashAddress,
  kEsfParameterStorageManagerItemSensorLoaderFlashAddress,
  kEsfParameterStorageManagerItemSensorFWFlashAddress,
  kEsfParameterStorageManagerItemAIModelParameterSlot0,
  kEsfParameterStorageManagerItemAIModelParameterSlot1,
  kEsfParameterStorageManagerItemAIModelParameterSlot2,
  kEsfParameterStorageManagerItemAIModelParameterSlot3,
  kEsfParameterStorageManagerItemAIModelParameterSlot4,
  kEsfParameterStorageManagerItemAIModelParameterHashSlot1,
  kEsfParameterStorageManagerItemAIModelParameterHashSlot2,
  kEsfParameterStorageManagerItemAIModelParameterHashSlot3,
  kEsfParameterStorageManagerItemAIModelParameterHashSlot4,
  kEsfParameterStorageManagerItemLMTStd,
  kEsfParameterStorageManagerItemPreWBStd,
  kEsfParameterStorageManagerItemGAMMAStd,
  kEsfParameterStorageManagerItemLSCStd,
  kEsfParameterStorageManagerItemLSCRawStd,
  kEsfParameterStorageManagerItemDEWARPStd,
  kEsfParameterStorageManagerItemLMTCustom,
  kEsfParameterStorageManagerItemPreWBCustom,
  kEsfParameterStorageManagerItemGAMMACustom,
  kEsfParameterStorageManagerItemGAMMAAutoCustom,
  kEsfParameterStorageManagerItemLSCCustom,
  kEsfParameterStorageManagerItemLSCRawCustom,
  kEsfParameterStorageManagerItemDEWARPCustom,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot0,
  kEsfParameterStorageManagerItemAIISPLoaderFlashAddress,
  kEsfParameterStorageManagerItemAIISPFWFlashAddress,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot1,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot2,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot3,
  kEsfParameterStorageManagerItemAIISPAIModelParameterSlot4,
  kEsfParameterStorageManagerItemAIModelSlotInfo,
  kEsfParameterStorageManagerItemAIISPAIModelSlotInfo,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo0,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo1,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo2,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo3,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo4,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo5,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo6,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo7,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo8,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo9,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo10,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo11,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo12,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo13,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo14,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo15,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo16,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo17,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo18,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo19,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo20,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo21,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo22,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo23,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo24,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo25,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo26,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo27,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo28,
  kEsfParameterStorageManagerItemFwMgrBinaryInfo29,
  kEsfParameterStorageManagerItemFwMgrBinaryInfoMcuFirmware,
  kEsfParameterStorageManagerItemEsfSensorConfig,
  kEsfParameterStorageManagerItemSpiBootLoader,
  kEsfParameterStorageManagerItemSpiBootFirmware,
  kEsfParameterStorageManagerItemSpiBootAIModel,
  kEsfParameterStorageManagerItemPreInstallAIModelInfo,
  kEsfParameterStorageManagerItemPreInstallAIModel,
  kEsfParameterStorageManagerItemInputTensorOnlyModel,
  kEsfParameterStorageManagerItemInputTensorOnlyParam,
  kEsfParameterStorageManagerItemExceptionFactor,
  kEsfParameterStorageManagerItemExceptionInfo,
  kEsfParameterStorageManagerItemEvpExceptionFactor,
  kEsfParameterStorageManagerItemCustom,
  kEsfParameterStorageManagerItemMax
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

// A structure that contains information about available Parameter Storage
// Manager capabilities.
typedef struct EsfParameterStorageManagerCapabilities {
  // If the API for canceling data updates is available, the value is 1.
  // If it is not available, the value is 0.
  uint32_t cancellable : 1;
} EsfParameterStorageManagerCapabilities;

// A structure that stores functional information about the data managed by
// Parameter Storage Manager.
typedef struct EsfParameterStorageManagerItemCapabilities {
  // In the case of 1, data cannot be changed by Save or Clear,
  // and data can only be retrieved by Load or GetSize.
  uint32_t read_only : 1;

  // If it is 1, the offset-specified binary data is available.
  // If it is 0, it is not available.
  uint32_t enable_offset : 1;
} EsfParameterStorageManagerItemCapabilities;

#ifdef __cplusplus
}
#endif

#endif  // ESF_PARAMETER_STORAGE_MANAGER_INCLUDE_PARAMETER_STORAGE_MANAGER_PARAMETER_STORAGE_MANAGER_COMMON_H_  // NOLINT
