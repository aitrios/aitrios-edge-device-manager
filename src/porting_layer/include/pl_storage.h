/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PORTING_LAYER_INCLUDE_PL_STORAGE_H_
#define PORTING_LAYER_INCLUDE_PL_STORAGE_H_
#include <fcntl.h>
#include <stdint.h>

#include "pl.h"

#define PL_STORAGE_OPEN_RDONLY (O_RDONLY)
#define PL_STORAGE_OPEN_WRONLY (O_WRONLY | O_CREAT | O_TRUNC)
#define PL_STORAGE_OPEN_RDWR (O_RDWR | O_CREAT)

typedef enum {
  PlStorageDataDpsURL,
  PlStorageDataCommonName,
  PlStorageDataDpsScopeID,
  PlStorageDataProjectID,
  PlStorageDataRegisterToken,
  PlStorageDataWiFiSSID,
  PlStorageDataWiFiPassword,
  PlStorageDataWiFiEncryption,
  PlStorageDataIPAddress,
  PlStorageDataSubnetMask,
  PlStorageDataGateway,
  PlStorageDataDNS,
  PlStorageDataIPMethod,
  PlStorageDataNetIfKind,
  PlStorageDataIPv6IPAddress,
  PlStorageDataIPv6SubnetMask,
  PlStorageDataIPv6Gateway,
  PlStorageDataIPv6DNS,
  PlStorageDataWiFiApSSID,
  PlStorageDataWiFiApPassword,
  PlStorageDataWiFiApEncryption,
  PlStorageDataWiFiApChannel,
  PlStorageDataWiFiApIPAddress,
  PlStorageDataWiFiApSubnetMask,
  PlStorageDataWiFiApGateway,
  PlStorageDataWiFiApDNS,
  PlStorageDataProxyURL,
  PlStorageDataProxyPort,
  PlStorageDataProxyUserName,
  PlStorageDataProxyPassword,
  PlStorageDataEvpHubURL,
  PlStorageDataEvpHubPort,
  PlStorageDataEvpIotPlatform,
  PlStorageDataPkiRootCerts,
  PlStorageDataPkiRootCertsHash,
  PlStorageDataEvpTls,
  PlStorageDataDeviceManifest,
  PlStorageDataDebugLogLevel,
  PlStorageDataEventLogLevel,
  PlStorageDataDebugLogDestination,
  PlStorageDataLogFilter,
  PlStorageDataLogUseFlash,
  PlStorageDataStorageName,
  PlStorageDataStorageSubDirectoryPath,
  PlStorageDataDebugLogLevel2,
  PlStorageDataEventLogLevel2,
  PlStorageDataDebugLogDestination2,
  PlStorageDataLogFilter2,
  PlStorageDataLogUseFlash2,
  PlStorageDataStorageName2,
  PlStorageDataStorageSubDirectoryPath2,
  PlStorageDataDebugLogLevel3,
  PlStorageDataEventLogLevel3,
  PlStorageDataDebugLogDestination3,
  PlStorageDataLogFilter3,
  PlStorageDataLogUseFlash3,
  PlStorageDataStorageName3,
  PlStorageDataStorageSubDirectoryPath3,
  PlStorageDataDebugLogLevel4,
  PlStorageDataEventLogLevel4,
  PlStorageDataDebugLogDestination4,
  PlStorageDataLogFilter4,
  PlStorageDataLogUseFlash4,
  PlStorageDataStorageName4,
  PlStorageDataStorageSubDirectoryPath4,
  PlStorageDataNTPServer,
  PlStorageDataNTPSyncInterval,
  PlStorageDataNTPPollingTime,
  PlStorageDataSkipModeSettings,
  PlStorageDataLimitPacketTime,
  PlStorageDataLimitRTCCorrectionValue,
  PlStorageDataSanityLimit,
  PlStorageDataSlewModeSettings,
  PlStorageDataStableRTCCorrectionValue,
  PlStorageDataStableSyncNumber,
  PlStorageDataSystemError,
  PlStorageDataFactoryResetFlag,
  PlStorageDataRTCErrorDetection,
  PlStorageDataRTCPQAParameter,
  PlStorageDataBatteryInformation,
  PlStorageDataRTCNetworkInformation,
  PlStorageDataRTCConfig,
  PlStorageDataHoursMeter,
  PlStorageDataSAS,
  PlStorageDataQRModeStateFlag,
  PlStorageDataInitialSettingFlag,
  PlStorageDataHWInfoText,
  PlStorageDataMCULoaderVersion,
  PlStorageDataSensorLoaderVersion,
  PlStorageDataMCUFWLastUpdate,
  PlStorageDataSensorLoaderLastUpdate,
  PlStorageDataSensorFWLastUpdate,
  PlStorageDataSensorAIModelFlashAddress,
  PlStorageDataSensorLoaderFlashAddress,
  PlStorageDataSensorFWFlashAddress,
  PlStorageDataAIModelParameterSlot0,
  PlStorageDataAIModelParameterSlot1,
  PlStorageDataAIModelParameterSlot2,
  PlStorageDataAIModelParameterSlot3,
  PlStorageDataAIModelParameterSlot4,
  PlStorageDataAIModelParameterHashSlot1,
  PlStorageDataAIModelParameterHashSlot2,
  PlStorageDataAIModelParameterHashSlot3,
  PlStorageDataAIModelParameterHashSlot4,
  PlStorageDataLMTStd,
  PlStorageDataPreWBStd,
  PlStorageDataGAMMAStd,
  PlStorageDataLSCStd,
  PlStorageDataLSCRawStd,
  PlStorageDataDEWARPStd,
  PlStorageDataLMTCustom,
  PlStorageDataPreWBCustom,
  PlStorageDataGAMMACustom,
  PlStorageDataGAMMAAutoCustom,
  PlStorageDataLSCCustom,
  PlStorageDataLSCRawCustom,
  PlStorageDataDEWARPCustom,
  PlStorageDataAIISPAIModelParameterSlot0,
  PlStorageDataAIISPLoaderFlashAddress,
  PlStorageDataAIISPFWFlashAddress,
  PlStorageDataAIISPAIModelParameterSlot1,
  PlStorageDataAIISPAIModelParameterSlot2,
  PlStorageDataAIISPAIModelParameterSlot3,
  PlStorageDataAIISPAIModelParameterSlot4,
  PlStorageDataAIModelSlotInfo,
  PlStorageDataAIISPAIModelSlotInfo,
  PlStorageDataFwMgrBinaryInfo0,
  PlStorageDataFwMgrBinaryInfo1,
  PlStorageDataFwMgrBinaryInfo2,
  PlStorageDataFwMgrBinaryInfo3,
  PlStorageDataFwMgrBinaryInfo4,
  PlStorageDataFwMgrBinaryInfo5,
  PlStorageDataFwMgrBinaryInfo6,
  PlStorageDataFwMgrBinaryInfo7,
  PlStorageDataFwMgrBinaryInfo8,
  PlStorageDataFwMgrBinaryInfo9,
  PlStorageDataFwMgrBinaryInfo10,
  PlStorageDataFwMgrBinaryInfo11,
  PlStorageDataFwMgrBinaryInfo12,
  PlStorageDataFwMgrBinaryInfo13,
  PlStorageDataFwMgrBinaryInfo14,
  PlStorageDataFwMgrBinaryInfo15,
  PlStorageDataFwMgrBinaryInfo16,
  PlStorageDataFwMgrBinaryInfo17,
  PlStorageDataFwMgrBinaryInfo18,
  PlStorageDataFwMgrBinaryInfo19,
  PlStorageDataFwMgrBinaryInfo20,
  PlStorageDataFwMgrBinaryInfo21,
  PlStorageDataFwMgrBinaryInfo22,
  PlStorageDataFwMgrBinaryInfo23,
  PlStorageDataFwMgrBinaryInfo24,
  PlStorageDataFwMgrBinaryInfo25,
  PlStorageDataFwMgrBinaryInfo26,
  PlStorageDataFwMgrBinaryInfo27,
  PlStorageDataFwMgrBinaryInfo28,
  PlStorageDataFwMgrBinaryInfo29,
  PlStorageDataFwMgrBinaryInfoMcuFirmware,
  PlStorageDataEsfSensorConfig,
  PlStorageDataSpiBootLoader,
  PlStorageDataSpiBootFirmware,
  PlStorageDataSpiBootAIModel,
  PlStorageDataPreInstallAIModelInfo,
  PlStorageDataPreInstallAIModel,
  PlStorageDataInputTensorOnlyModel,
  PlStorageDataInputTensorOnlyParam,
  PlStorageDataExceptionFactor,
  PlStorageDataExceptionInfo,
  PlStorageDataEvpExceptionFactor,
  PlStorageDataMax
} PlStorageDataId;

typedef uint32_t PlStorageTmpDataId;

typedef void *PlStorageHandle;

typedef enum {
  PlStorageSeekSet,
  PlStorageSeekCur,
  PlStorageSeekEnd,
  PlStorageSeekMax
} PlStorageSeekType;

typedef struct {
  uint32_t written_size;
} PlStorageDataInfo;

typedef struct {
  uint32_t enable_tmp_id : 1;
  uint32_t reserve : 31;
} PlStorageCapabilities;

typedef struct {
  uint32_t is_read_only : 1;
  uint32_t enable_seek : 1;
  uint32_t reserve : 29;
} PlStorageIdCapabilities;

// Functions ------------------------------------------------------------------
PlErrCode PlStorageOpen(PlStorageDataId id, int oflags,
                        PlStorageHandle *handle);
PlErrCode PlStorageClose(const PlStorageHandle handle);
PlErrCode PlStorageSeek(const PlStorageHandle handle, int32_t offset,
                        PlStorageSeekType type, int32_t *cur_pos);
PlErrCode PlStorageRead(const PlStorageHandle handle, void *out_buf,
                        uint32_t read_size, uint32_t *out_size);
PlErrCode PlStorageWrite(const PlStorageHandle handle, const void *src_buf,
                         uint32_t write_size, uint32_t *out_size);
PlErrCode PlStorageErase(PlStorageDataId id);
PlErrCode PlStorageFactoryReset(PlStorageDataId id);
PlErrCode PlStorageDRead(PlStorageDataId id, int oflags, void *out_buf,
                         uint32_t read_size, uint32_t *out_size);
PlErrCode PlStorageDWrite(PlStorageDataId id, int oflags, const void *src_buf,
                          uint32_t write_size, uint32_t *out_size);
PlErrCode PlStorageGetDataInfo(PlStorageDataId id, PlStorageDataInfo *info);
PlErrCode PlStorageSwitchData(PlStorageTmpDataId src_id,
                              PlStorageDataId dst_id);
PlErrCode PlStorageGetTmpDataId(PlStorageDataId src_id,
                                PlStorageTmpDataId *tmp_id);
PlErrCode PlStorageGetCapabilities(PlStorageCapabilities *capabilities);
PlErrCode PlStorageGetIdCapabilities(PlStorageDataId id,
                                     PlStorageIdCapabilities *id_capabilities);
PlErrCode PlStorageInitialize(void);
PlErrCode PlStorageFinalize(void);
PlErrCode PlStorageClean(void);
PlErrCode PlStorageDowngrade(void);
#endif  // PORTING_LAYER_INCLUDE_PL_STORAGE_H_
