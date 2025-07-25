/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_SETTINGS_TABLE_H_  /* NOLINT */
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_SETTINGS_TABLE_H_  /* NOLINT */

#include "parameter_storage_manager.h"
#include "parameter_storage_manager/src/parameter_storage_manager_config.h"
#include "parameter_storage_manager/src/parameter_storage_manager_storage_adapter_settings.h"  /* NOLINT */

// Union that stores the data ID of each data storage area.
typedef union EsfParameterStorageManagerStorageAdapterDataID {
  // PL Storage's data id.
  PlStorageDataId pl_storage;
  // other's data id.
  EsfParameterStorageManagerStorageOtherDataID other;
} EsfParameterStorageManagerStorageAdapterDataID;

// Structure for mapping IDs for each EsfParameterStorageManagerItemID.
typedef struct EsfParameterStorageManagerStorageAdapterDataInfo {
  // This member identifies the data storage area.
  EsfParameterStorageManagerStorageID storage;
  // A data ID unique to the data storage area.
  EsfParameterStorageManagerStorageAdapterDataID data_id;
  // A flag that indicates whether this data should be restored to its factory
  // settings. If true, it will be restored to its factory settings.
  bool factory_reset_required;
} EsfParameterStorageManagerStorageAdapterDataInfo;

// Data ID management table
static const EsfParameterStorageManagerStorageAdapterDataInfo
  kDataIdTable[kEsfParameterStorageManagerItemMax] = {
    [kEsfParameterStorageManagerItemDpsURL] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDpsURL},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemCommonName] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataCommonName},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDpsScopeID] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDpsScopeID},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemProjectID] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataProjectID},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemRegisterToken] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataRegisterToken},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiSSID] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiSSID},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiPassword] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiPassword},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiEncryption] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiEncryption},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemIPAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataIPAddress},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSubnetMask] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSubnetMask},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemGateway] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataGateway},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDNS] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDNS},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemIPMethod] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataIPMethod},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemNetIfKind] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataNetIfKind},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemIPv6IPAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataIPv6IPAddress},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemIPv6SubnetMask] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataIPv6SubnetMask},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemIPv6Gateway] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataIPv6Gateway},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemIPv6DNS] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataIPv6DNS},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiApSSID] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApSSID},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemWiFiApPassword] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApPassword},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemWiFiApEncryption] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApEncryption},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiApChannel] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApChannel},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiApIPAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApIPAddress},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiApSubnetMask] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApSubnetMask},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiApGateway] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApGateway},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemWiFiApDNS] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataWiFiApDNS},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemProxyURL] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataProxyURL},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemProxyPort] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataProxyPort},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemProxyUserName] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataProxyUserName},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemProxyPassword] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataProxyPassword},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEvpHubURL] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEvpHubURL},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEvpHubPort] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEvpHubPort},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEvpIotPlatform] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEvpIotPlatform},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemPkiRootCerts] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataPkiRootCerts},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemPkiRootCertsHash] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataPkiRootCertsHash},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemEvpTls] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEvpTls},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDeviceManifest] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDeviceManifest},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemDebugLogLevel] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogLevel},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEventLogLevel] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEventLogLevel},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogDestination] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogDestination},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogFilter] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogFilter},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogUseFlash] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogUseFlash},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageName] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageName},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageSubDirectoryPath] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageSubDirectoryPath},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogLevel2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogLevel2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEventLogLevel2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEventLogLevel2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogDestination2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogDestination2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogFilter2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogFilter2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogUseFlash2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogUseFlash2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageName2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageName2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageSubDirectoryPath2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageSubDirectoryPath2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogLevel3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogLevel3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEventLogLevel3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEventLogLevel3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogDestination3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogDestination3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogFilter3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogFilter3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogUseFlash3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogUseFlash3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageName3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageName3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageSubDirectoryPath3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageSubDirectoryPath3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogLevel4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogLevel4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEventLogLevel4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEventLogLevel4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDebugLogDestination4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDebugLogDestination4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogFilter4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogFilter4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLogUseFlash4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLogUseFlash4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageName4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageName4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStorageSubDirectoryPath4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStorageSubDirectoryPath4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemNTPServer] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataNTPServer},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemNTPSyncInterval] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataNTPSyncInterval},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemNTPPollingTime] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataNTPPollingTime},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSkipModeSettings] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSkipModeSettings},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLimitPacketTime] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLimitPacketTime},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLimitRTCCorrectionValue] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLimitRTCCorrectionValue},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSanityLimit] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSanityLimit},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSlewModeSettings] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSlewModeSettings},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStableRTCCorrectionValue] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStableRTCCorrectionValue},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemStableSyncNumber] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataStableSyncNumber},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSystemError] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSystemError},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFactoryResetFlag] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFactoryResetFlag},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemRTCErrorDetection] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataRTCErrorDetection},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemRTCPQAParameter] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataRTCPQAParameter},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemBatteryInformation] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataBatteryInformation},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemRTCNetworkInformation] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataRTCNetworkInformation},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemRTCConfig] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataRTCConfig},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemHoursMeter] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataHoursMeter},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSAS] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSAS},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemQRModeStateFlag] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataQRModeStateFlag},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemInitialSettingFlag] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataInitialSettingFlag},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemHWInfoText] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataHWInfoText},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemMCULoaderVersion] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataMCULoaderVersion},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSensorLoaderVersion] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSensorLoaderVersion},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemMCUFWLastUpdate] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataMCUFWLastUpdate},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSensorLoaderLastUpdate] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSensorLoaderLastUpdate},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSensorFWLastUpdate] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSensorFWLastUpdate},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemSensorAIModelFlashAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSensorAIModelFlashAddress},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSensorLoaderFlashAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSensorLoaderFlashAddress},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSensorFWFlashAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSensorFWFlashAddress},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemAIModelParameterSlot0] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterSlot0},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemAIModelParameterSlot1] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterSlot1},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterSlot2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterSlot2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterSlot3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterSlot3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterSlot4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterSlot4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterHashSlot1] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterHashSlot1},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterHashSlot2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterHashSlot2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterHashSlot3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterHashSlot3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelParameterHashSlot4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelParameterHashSlot4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLMTStd] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLMTStd},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemPreWBStd] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataPreWBStd},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemGAMMAStd] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataGAMMAStd},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemLSCStd] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLSCStd},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemLSCRawStd] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLSCRawStd},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemDEWARPStd] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDEWARPStd},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemLMTCustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLMTCustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemPreWBCustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataPreWBCustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemGAMMACustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataGAMMACustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemGAMMAAutoCustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataGAMMAAutoCustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLSCCustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLSCCustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemLSCRawCustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataLSCRawCustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemDEWARPCustom] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataDEWARPCustom},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPAIModelParameterSlot0] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPAIModelParameterSlot0},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemAIISPLoaderFlashAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPLoaderFlashAddress},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPFWFlashAddress] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPFWFlashAddress},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPAIModelParameterSlot1] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPAIModelParameterSlot1},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPAIModelParameterSlot2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPAIModelParameterSlot2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPAIModelParameterSlot3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPAIModelParameterSlot3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPAIModelParameterSlot4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPAIModelParameterSlot4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIModelSlotInfo] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIModelSlotInfo},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemAIISPAIModelSlotInfo] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataAIISPAIModelSlotInfo},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo0] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo0},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo1] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo1},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo2] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo2},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo3] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo3},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo4] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo4},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo5] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo5},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo6] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo6},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo7] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo7},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo8] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo8},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo9] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo9},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo10] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo10},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo11] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo11},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo12] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo12},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo13] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo13},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo14] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo14},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo15] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo15},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo16] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo16},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo17] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo17},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo18] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo18},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo19] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo19},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo20] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo20},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo21] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo21},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo22] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo22},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo23] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo23},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo24] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo24},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo25] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo25},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo26] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo26},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo27] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo27},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo28] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo28},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfo29] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfo29},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemFwMgrBinaryInfoMcuFirmware] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataFwMgrBinaryInfoMcuFirmware},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemEsfSensorConfig] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEsfSensorConfig},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSpiBootLoader] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSpiBootLoader},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSpiBootFirmware] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSpiBootFirmware},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemSpiBootAIModel] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataSpiBootAIModel},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemPreInstallAIModelInfo] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataPreInstallAIModelInfo},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemPreInstallAIModel] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataPreInstallAIModel},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemInputTensorOnlyModel] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataInputTensorOnlyModel},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemInputTensorOnlyParam] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataInputTensorOnlyParam},
      .factory_reset_required = false,
    },
    [kEsfParameterStorageManagerItemExceptionFactor] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataExceptionFactor},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemExceptionInfo] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataExceptionInfo},
      .factory_reset_required = true,
    },
    [kEsfParameterStorageManagerItemEvpExceptionFactor] = {
      .storage = kEsfParameterStorageManagerStoragePl,
      .data_id = {.pl_storage = PlStorageDataEvpExceptionFactor},
      .factory_reset_required = true,
    },
};

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_STORAGE_ADAPTER_SETTINGS_TABLE_H_  /* NOLINT */
