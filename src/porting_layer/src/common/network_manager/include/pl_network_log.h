/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_NETWORK_LOG_H_
#define PL_NETWORK_LOG_H_

// Includes --------------------------------------------------------------------
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ----------------------------------------------------------------------
#define BASE (MODULE_ID_SYSTEM)
#define DLOGE(fmt, ...)                                                      \
  do {                                                                       \
    WRITE_DLOG_ERROR(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGW(fmt, ...)                                                     \
  do {                                                                      \
    WRITE_DLOG_WARN(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGI(fmt, ...)                                                     \
  do {                                                                      \
    WRITE_DLOG_INFO(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGD(fmt, ...)                                                      \
  do {                                                                       \
    WRITE_DLOG_DEBUG(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define ELOGE(event_id)               \
  do {                                \
    WRITE_ELOG_ERROR(BASE, event_id); \
  } while (0)

#define ELOGW(event_id)              \
  do {                               \
    WRITE_ELOG_WARN(BASE, event_id); \
  } while (0)

// Types (typedef / enum / struct / union) -------------------------------------

// To be adjusted later
typedef enum {
  kElog_PlErrInvalidParam = 0x9600,
  kElog_PlErrInvalidState = 0x9601,
  kElog_PlErrInvalidOperation = 0x9602,
  kElog_PlErrLock = 0x9603,
  kElog_PlErrUnlock = 0x9604,
  kElog_PlErrAlready = 0x9605,
  kElog_PlErrNotFound = 0x9606,
  kElog_PlErrNoSupported = 0x9607,
  kElog_PlErrMemory = 0x9608,
  kElog_PlErrInternal = 0x9609,
  kElog_PlErrConfig = 0x960A,
  kElog_PlErrInvalidValue = 0x960B,
  kElog_PlErrHandler = 0x960C,
  kElog_PlErrIrq = 0x960D,
  kElog_PlErrCallback = 0x960E,
  kElog_PlThreadError = 0x960F,
  kElog_PlErrOpen = 0x9610,
  kElog_PlErrInputDirection = 0x9611,
  kElog_PlErrClose = 0x9612,
  kElog_PlErrDevice = 0x9613,
  kElog_PlErrMagicCode = 0x9614,
  kElog_PlErrBufferOverflow = 0x9615,
  kElog_PlErrWrite = 0x9616,
  kElog_PlErrTimeout = 0x9617,
  kElog_PlEtherFinalize = 0x9618,
  kElog_PlEtherGetStatus = 0x9619,
  kElog_PlEtherInitialize = 0x961A,
  kElog_PlEtherLan9250Finalize = 0x961B,
  kElog_PlEtherLan9250Initialize = 0x961C,
  kElog_PlEtherStart = 0x961D,
  kElog_PlEtherStop = 0x961E,
  kElog_PlNetworkEventThread = 0x961F,
  kElog_PlNetworkFinalize = 0x9620,
  kElog_PlNetworkGetConfig = 0x9621,
  kElog_PlNetworkGetIfStatusOsImpl = 0x9622,
  kElog_PlNetworkGetIpv4AddrOsImpl = 0x9623,
  kElog_PlNetworkGetIpv4DnsOsImpl = 0x9624,
  kElog_PlNetworkGetIpv4GatewayOsImpl = 0x9625,
  kElog_PlNetworkGetLinkStatusOsImpl = 0x9626,
  kElog_PlNetworkGetMigrationDataOsImpl = 0x9627,
  kElog_PlNetworkGetNetStat = 0x9628,
  kElog_PlNetworkGetStatus = 0x9629,
  kElog_PlNetworkGetSystemInfo = 0x962A,
  kElog_PlNetworkInitialize = 0x962B,
  kElog_PlNetworkInitMigrationOsImpl = 0x962C,
  kElog_PlNetworkIsNeedMigration = 0x962D,
  kElog_PlNetworkRegisterEventHandler = 0x962E,
  kElog_PlNetworkSetConfig = 0x962F,
  kElog_PlNetworkSetIfStatusOsImpl = 0x9630,
  kElog_PlNetworkSetIpv4AddrOsImpl = 0x9631,
  kElog_PlNetworkSetIpv4DnsOsImpl = 0x9632,
  kElog_PlNetworkSetIpv4GatewayOsImpl = 0x9633,
  kElog_PlNetworkSetIpv4MethodOsImpl = 0x9634,
  kElog_PlNetworkStart = 0x9635,
  kElog_PlNetworkStop = 0x9636,
  kElog_PlNetworkStructInitialize = 0x9637,
  kElog_PlNetworkUnregisterEventHandler = 0x9638,
  kElog_PlWifiApGetConfig = 0x9639,
  kElog_PlWifiApGetStatus = 0x963A,
  kElog_PlWifiApRegisterEventHandler = 0x963B,
  kElog_PlWifiApSetConfig = 0x963C,
  kElog_PlWifiApStart = 0x963D,
  kElog_PlWifiApStop = 0x963E,
  kElog_PlWifiApUnregisterEventHandler = 0x963F,
  kElog_PlWifiFinalize = 0x9640,
  kElog_PlWifiGetConfig = 0x9641,
  kElog_PlWifiGetStatus = 0x9642,
  kElog_PlWifiInitialize = 0x9643,
  kElog_PlWifiRegisterEventHandler = 0x9644,
  kElog_PlWifiSetConfig = 0x9645,
  kElog_PlWifiStaGetConfigOsImpl = 0x9646,
  kElog_PlWifiStaGetStatusOsImpl = 0x9647,
  kElog_PlWifiStaRegisterEventHandlerOsImpl = 0x9648,
  kElog_PlWifiStart = 0x9649,
  kElog_PlWifiStaSetConfigOsImpl = 0x964A,
  kElog_PlWifiStaStartOsImpl = 0x964B,
  kElog_PlWifiStaStopOsImpl = 0x964C,
  kElog_PlWifiStaUnregisterEventHandlerOsImpl = 0x964D,
  kElog_PlWifiStop = 0x964E,
  kElog_PlWifiUnregisterEventHandler = 0x964F,
  kElog_NetworkCreateEventThread = 0x9650,
  kElog_NetworkDeviceFinalize = 0x9651,
  kElog_NetworkDeviceInitialize = 0x9652,
  kElog_EtherMonitorThread = 0x9653,
  kElog_EtherCreateMonitorThread = 0x9654,
  kElog_EtherDestroyMonitorThread = 0x9655,
  kElog_PlEtherInitializeCamImpl = 0x9656,
  kElog_PlEtherFinalizeCamImpl = 0x9657,
  kElog_Lan9250IrqAttach = 0x9658,
  kElog_SpiInitialize = 0x9659,
  kElog_PowerPortSetting = 0x965A,
  kElog_ResetPortSetting = 0x965B,
  kElog_PortConfigure = 0x965C,
  kElog_PortSetValue = 0x965D,
  kElog_ParseStringToInt32 = 0x965E,
  kElog_ParseIPAddress = 0x965F,
  kElog_ParseSubnetMask = 0x9660,
  kElog_NmGetConnection = 0x9661,
  kElog_NmGetIpv4Config = 0x9662,
  kElog_NmUpdateConnectionCallback = 0x9663,
  kElog_NmActivateConnectionCallback = 0x9664,
  kElog_NmUpdateConnection = 0x9665,
  kElog_NmSetIpv4addr = 0x9666,
  kElog_NmSetIpv4Gateway = 0x9667,
  kElog_NmSetIpv4dnsaddr = 0x9668,
  kElog_NmSetIpv4method = 0x9669,
  kElog_NmGetIpv4addr = 0x966A,
  kElog_NmGetIpv4netmask = 0x966B,
  kElog_NmGetIpv4Gateway = 0x966C,
  kElog_NmGetIpv4dnsaddr = 0x966D,
  kElog_ReadLine = 0x966E,
  kElog_ReadFile = 0x966F,
  kElog_open = 0x9670,
  kElog_read = 0x9671,
  kElog_close = 0x9672,
  kElog_socket = 0x9673,
  kElog_SIOCSIFFLAGS = 0x9674,
  kElog_SIOCGIFFLAGS = 0x9675,
  kElog_SIOCGMIIPHY = 0x9676,
  kElog_MII_MSR = 0x9677,
  kElog_MII_PHYID1 = 0x9678,
  kElog_MII_PHYID2 = 0x9679,
  kElog_MII_BMSR = 0x967A,
  kElog_MII_PHYSID1 = 0x967B,
  kElog_MII_PHYSID2 = 0x967C,
} PlNetworkElog;

#endif  // PL_NETWORK_LOG_H_
