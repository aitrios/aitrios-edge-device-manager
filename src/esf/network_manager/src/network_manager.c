/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// self header.
#include "network_manager.h"

// system header.
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// other header.
#include "led_manager.h"
#include "network_manager/network_manager_accessor.h"
#include "network_manager/network_manager_accessor_parameter_storage_manager.h"
#include "network_manager/network_manager_internal.h"
#include "network_manager/network_manager_log.h"
#include "network_manager/network_manager_resource.h"
#include "porting_layer/include/pl.h"
#include "porting_layer/include/pl_network.h"

// """One parameter sanity check function type.

// One parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
typedef bool (*EsfNetworkManagerValidateOneParameter)(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
static bool EsfNetworkManagerValidateParameterInternalIpaddress(
    int af, const char *addr);
static bool EsfNetworkManagerValidateOneParameterDevIpIp(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpMask(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpGateway(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpDns(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpDns2(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpv6Ip(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpv6Mask(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpv6Gateway(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterDevIpv6Dns(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterWifiStaSsid(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterWifiStaPassword(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterWifiStaEncryption(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterIpMethod(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterNetifKind(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpIp(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpMask(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpGateway(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpDns(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointWifiApSsid(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointWifiApPassword(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterAccessPointWifiApEncryption(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterProxyUrl(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterProxyPort(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterProxyUsername(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateOneParameterProxyPassword(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateParameterInternal(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect);
static bool EsfNetworkManagerValidateConnectInfo(
    const EsfNetworkManagerHandleInternal *handle_internal,
    const EsfNetworkManagerOSInfo *os_info);
static void EsfNetworkManagerUpdateStopStatus(
    EsfNetworkManagerHandleInternal *handle_internal);
static EsfNetworkManagerResult EsfNetworkManagerGetInterfaceForGetIFStatus(
    const EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerInterfaceKind *interface_kind);
static EsfNetworkManagerResult EsfNetworkManagerLoadConnectInfo(
    const EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerOSInfo *os_info);
static EsfNetworkManagerResult EsfNetworkManagerGetStartConnectInfo(
    const EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerStartType start_type, EsfNetworkManagerOSInfo *os_info,
    EsfNetworkManagerOSInfo **start_connection, bool *need_free);
static EsfNetworkManagerResult EsfNetworkManagerPreStart(
    EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerStartType start_type, EsfNetworkManagerOSInfo *os_info,
    EsfNetworkManagerOSInfo **start_connection, bool *need_free);
static EsfNetworkManagerResult EsfNetworkManagerAfterStart(
    EsfNetworkManagerHandleInternal *handle_internal,
    const EsfNetworkManagerOSInfo *start_connection);
static void EsfNetworkManagerShowConnectInfo(
    EsfNetworkManagerMode mode, const EsfNetworkManagerOSInfo *os_info);
static void EsfNetworkManagerShowParameter(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter);
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

// The minimum port number.
#define ESF_NETWORK_MANAGER_PORT_MIN (0)
// The maximum port number.
#define ESF_NETWORK_MANAGER_PORT_MAX (65535)

EsfNetworkManagerResult EsfNetworkManagerInit(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  do {
    EsfNetworkManagerAccessorInit();
    uint32_t info_total_num = 0;
    PlNetworkSystemInfo *infos = NULL;
    ret = EsfNetworkManagerAccessorGetSystemInfo(&info_total_num, &infos);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Get PL network system info error.(ret=%u)", ret);
      break;
    }
    ret = EsfNetworkManagerResourceInit(info_total_num, infos);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource initialize error.(ret=%u)", ret);
      break;
    }
  } while (0);

  if (ret != kEsfNetworkManagerResultSuccess) {
    EsfNetworkManagerAccessorDeinit();
  } else {
    ESF_NETWORK_MANAGER_INFO("NetworkManager Initialize success.");
  }
#else
  ret = EsfNetworkManagerResourceInit(0, NULL);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource initialize error.(ret=%u)", ret);
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ESF_NETWORK_MANAGER_TRACE("END");
  return ret;
}

EsfNetworkManagerResult EsfNetworkManagerDeinit(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  EsfNetworkManagerResult ret_result = kEsfNetworkManagerResultSuccess;
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  do {
    EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
    ret = EsfNetworkManagerResourceLockResourceNoTimeout();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      ret_result = ret;
      break;
    }
    char *if_name_ether = NULL;
    char *if_name_wifist = NULL;
    char *if_name_wifiap = NULL;
    ret = EsfNetworkManagerResourceGetIfname(&if_name_ether, &if_name_wifist,
                                             &if_name_wifiap);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_WARN("Get resource interface name error.");
      ret_result = ret;
      // Fall through due to exclusive release.
    }
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      ret_result = ret;
      break;
    }
    if (ret_result != kEsfNetworkManagerResultSuccess) {
      break;
    }
    EsfNetworkManagerAccessorStopAllNetwork(if_name_ether, if_name_wifist,
                                            if_name_wifiap);
    EsfNetworkManagerAccessorUnregisterAllEventHandler(
        if_name_ether, if_name_wifist, if_name_wifiap);
    EsfNetworkManagerAccessorDeinit();
    ret = EsfNetworkManagerResourceDeinit();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_WARN("Resource deinit error.");
      ret_result = ret;
      break;
    } else {
      ESF_NETWORK_MANAGER_INFO("NetworkManager Deinitialize success.");
    }
  } while (0);
#else
  ret_result = EsfNetworkManagerResourceDeinit();
  if (ret_result != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_WARN("Resource deinit error.");
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ESF_NETWORK_MANAGER_TRACE("END");
  return ret_result;
}

EsfNetworkManagerResult EsfNetworkManagerOpen(
    EsfNetworkManagerMode mode, EsfNetworkManagerHandleType handle_type,
    EsfNetworkManagerHandle *handle) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%d, handle_type=%d, handle=%p", mode,
                            handle_type, handle);
  if (handle == NULL || mode < 0 || mode >= kEsfNetworkManagerModeNum ||
      handle_type < 0 || handle_type >= kEsfNetworkManagerHandleTypeNum) {
    ESF_NETWORK_MANAGER_ERR(
        "Parameter error. mode=%u, handle_type=%u, handle=%p", mode,
        handle_type, handle);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  ESF_NETWORK_MANAGER_DBG("Open info mode=%d, handle_type=%d, handle=%p", mode,
                          handle_type, handle);
  // Disable AccessPointMode.
  if (mode == kEsfNetworkManagerModeAccessPoint) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mode=%u", mode);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }

  EsfNetworkManagerResult ret =
      EsfNetworkManagerResourceNewHandle(mode, handle_type, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle new error. ret=%u", ret);
    return ret;
  }
  ESF_NETWORK_MANAGER_DBG("Open success. handle=%d", *handle);
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerClose(EsfNetworkManagerHandle handle) {
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32, handle);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32, handle);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  ESF_NETWORK_MANAGER_DBG("Close info. handle=%d", handle);
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceDeleteHandle(handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle delete error. ret=%u", ret);
    return ret;
  }
  ESF_NETWORK_MANAGER_DBG("Close Success. handle=%d", handle);
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerStart(
    EsfNetworkManagerHandle handle, EsfNetworkManagerStartType start_type,
    EsfNetworkManagerOSInfo *os_info) {
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32 " start_type=%" PRId32
                            " os_info=%p",
                            handle, start_type, os_info);
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerAccessorLedManagerStatus led_manager_status =
      kEsfNetworkManagerAccessorLedManagerStatusConnecting;
  EsfNetworkManagerOSInfo *start_connection = NULL;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerMode mode = kEsfNetworkManagerModeNormal;
  EsfNetworkManagerNetifKind netif_kind = kEsfNetworkManagerNetifKindEther;
  bool need_free = false;
  bool is_locked = false;
  do {
    if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
      ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32, handle);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInvalidParameter;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
      break;
    }
    ret = EsfNetworkManagerResourceOpenHandle(true, handle, &handle_internal);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
      break;
    }
    ret = EsfNetworkManagerResourceLockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
      break;
    }
    is_locked = true;

    ESF_NETWORK_MANAGER_DBG("Network start mode=%d start_type=%" PRId32,
                            handle_internal->mode, start_type);
    mode = handle_internal->mode;

    ret = EsfNetworkManagerPreStart(handle_internal, start_type, os_info,
                                    &start_connection, &need_free);
    if (ret != kEsfNetworkManagerResultSuccess) {
      // Aready running is warning level log
      ESF_NETWORK_MANAGER_WARN("EsfNetworkManagerPreStart error.");
      if (ret == kEsfNetworkManagerResultInvalidParameter) {
        led_manager_status =
            kEsfNetworkManagerAccessorLedManagerStatusParameterError;
      } else {
        // In case of other errors.
        led_manager_status =
            kEsfNetworkManagerAccessorLedManagerStatusStartError;
      }
      return_result = ret;
      break;
    }
    netif_kind =
        (EsfNetworkManagerNetifKind)start_connection->normal_mode.netif_kind;

    ret = EsfNetworkManagerAccessorStart(start_connection, handle_internal);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Network start error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
      break;
    }

    ret = EsfNetworkManagerAfterStart(handle_internal, start_connection);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("EsfNetworkManagerAfterStart error.");
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
      return_result = ret;
      break;
    }
    ESF_NETWORK_MANAGER_INFO(
        "Start Network %s.",
        (netif_kind == kEsfNetworkManagerNetifKindWiFi) ? "WiFi" : "Ether");
  } while (0);
  if (is_locked) {
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
    }
  }
  if (handle_internal != NULL) {
    ret = EsfNetworkManagerResourceCloseHandle(true, handle);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Handle close error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStartError;
    }
  }

  EsfNetworkManagerAccessorSetLedManagerStatus(mode, led_manager_status,
                                               netif_kind);
  if (return_result == kEsfNetworkManagerResultSuccess) {
    EsfNetworkManagerAccessorSetLedManagerStatusService(
        kEsfLedManagerLedStatusDisconnectedEstablishingNetworkLinkOnPhysicalLayer,
        true);
  } else {
    EsfNetworkManagerAccessorSetLedManagerStatus(
        mode, kEsfNetworkManagerAccessorLedManagerStatusDisconnected,
        netif_kind);
  }

  if (need_free && start_connection != NULL) {
    free(start_connection);
  }
#else
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32, handle);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }

  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult ret =
      EsfNetworkManagerResourceOpenHandle(true, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }

  handle_internal->mode_info->notify_info =
      kEsfNetworkManagerNotifyInfoConnected;
  if (handle_internal->mode_info->callback != NULL) {
    EsfNetworkManagerAccessorCallCallback(handle_internal->mode,
                                          handle_internal->mode_info);
  }

  ret = EsfNetworkManagerResourceCloseHandle(true, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error. ret=%u", ret);
    return_result = ret;
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerStop(EsfNetworkManagerHandle handle) {
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32, handle);
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerAccessorLedManagerStatus led_manager_status =
      kEsfNetworkManagerAccessorLedManagerStatusIgnore;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerMode mode = kEsfNetworkManagerModeNormal;
  EsfNetworkManagerNetifKind netif_kind = kEsfNetworkManagerNetifKindEther;
  bool is_locked = false;
  do {
    if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
      ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32, handle);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInvalidParameter;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStopError;
      break;
    }
    ret = EsfNetworkManagerResourceOpenHandle(true, handle, &handle_internal);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStopError;
      break;
    }
    mode = handle_internal->mode;
    netif_kind =
        (EsfNetworkManagerNetifKind)
            handle_internal->mode_info->connect_info.normal_mode.netif_kind;
    ESF_NETWORK_MANAGER_TRACE("Network stop mode=%d netif_kind=%d", mode,
                              netif_kind);
    ret = EsfNetworkManagerResourceLockResourceNoTimeout();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStopError;
      break;
    }
    is_locked = true;
    if (handle_internal->mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusDisconnected ||
        handle_internal->mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusDisconnecting) {
      ESF_NETWORK_MANAGER_WARN(
          "Connection status is not connected or connecting status=%u.",
          handle_internal->mode_info->connect_status);
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      return_result = kEsfNetworkManagerResultStatusAlreadyRunning;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusIgnore;
      break;
    }
    ret = EsfNetworkManagerAccessorStop(handle_internal);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Network stop error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStopError;
      break;
    }
    EsfNetworkManagerUpdateStopStatus(handle_internal);
    if (handle_internal->mode_info->connect_status ==
        kEsfNetworkManagerConnectStatusDisconnecting) {
      led_manager_status =
          kEsfNetworkManagerAccessorLedManagerStatusDisconnecting;
    } else {
      led_manager_status =
          kEsfNetworkManagerAccessorLedManagerStatusDisconnected;
    }
    memset(&handle_internal->mode_info->ip_info, 0,
           sizeof(handle_internal->mode_info->ip_info));
    ESF_NETWORK_MANAGER_INFO(
        "Stop Network %s.",
        (netif_kind == kEsfNetworkManagerNetifKindWiFi) ? "WiFi" : "Ether");
  } while (0);
  if (is_locked) {
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStopError;
    }
  }
  if (handle_internal != NULL) {
    ret = EsfNetworkManagerResourceCloseHandle(true, handle);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Handle close error.");
      return_result = ret;
      led_manager_status = kEsfNetworkManagerAccessorLedManagerStatusStopError;
    }
  }
  EsfNetworkManagerAccessorSetLedManagerStatus(mode, led_manager_status,
                                               netif_kind);
  if (return_result == kEsfNetworkManagerResultSuccess) {
    EsfNetworkManagerAccessorSetLedManagerStatusService(
        kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, false);
    EsfNetworkManagerAccessorSetLedManagerStatusService(
        kEsfLedManagerLedStatusDisconnectedNoInternetConnection, false);
    EsfNetworkManagerAccessorSetLedManagerStatusService(
        kEsfLedManagerLedStatusDisconnectedEstablishingNetworkLinkOnPhysicalLayer,
        false);
  }
#else
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32, handle);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }

  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult ret =
      EsfNetworkManagerResourceOpenHandle(true, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }

  handle_internal->mode_info->notify_info =
      kEsfNetworkManagerNotifyInfoDisconnected;
  if (handle_internal->mode_info->callback != NULL) {
    EsfNetworkManagerAccessorCallCallback(handle_internal->mode,
                                          handle_internal->mode_info);
  }

  ret = EsfNetworkManagerResourceCloseHandle(true, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error. ret=%u", ret);
    return_result = ret;
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerGetIFStatus(
    EsfNetworkManagerHandle handle, EsfNetworkManagerStatusInfo *status) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32 ", status=%p", handle,
                            status);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX ||
      status == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32 ", status=%p",
                            handle, status);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  bool is_locked = false;

  ret = EsfNetworkManagerResourceOpenHandle(false, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }
  do {
    ret = EsfNetworkManagerResourceLockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      break;
    }
    is_locked = true;
    EsfNetworkManagerInterfaceKind interface_kind =
        kEsfNetworkManagerInterfaceKindWifi;
    ret = EsfNetworkManagerGetInterfaceForGetIFStatus(handle_internal,
                                                      &interface_kind);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Get Interface error.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = ret;
      break;
    }

    // if_name is accessible until the PL terminates.
    char *if_name =
        handle_internal->mode_info->hal_system_info[interface_kind]->if_name;
    is_locked = false;
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
      break;
    }
    ret = EsfNetworkManagerAccessorGetStatusInfo(if_name, status);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Network status get error. ret=%u", ret);
      return_result = ret;
      break;
    }
    ESF_NETWORK_MANAGER_DBG("Network status %s if_status:%d link_status:%d",
                            if_name, status->is_if_up, status->is_link_up);
  } while (0);
  if (is_locked) {
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
    }
  }
  ret = EsfNetworkManagerResourceCloseHandle(false, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error.");
    return_result = ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  status->is_if_up = true;
  status->is_link_up = true;
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerGetIFInfo(
    EsfNetworkManagerHandle handle, EsfNetworkManagerOSInfo *ifinfo) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32 ", ifinfo=%p", handle,
                            ifinfo);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX ||
      ifinfo == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32 ", ifinfo=%p",
                            handle, ifinfo);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  bool is_locked = false;

  ret = EsfNetworkManagerResourceOpenHandle(false, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }
  do {
    ret = EsfNetworkManagerResourceLockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      break;
    }
    is_locked = true;
    if (!handle_internal->mode_info->is_connect_info_saved) {
      ESF_NETWORK_MANAGER_ERR("No connect informations.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultNoConnectInfo;
      break;
    }
    *ifinfo = handle_internal->mode_info->connect_info;
    if (handle_internal->mode == kEsfNetworkManagerModeNormal &&
        handle_internal->mode_info->connect_info.normal_mode.ip_method ==
            kEsfNetworkManagerIpMethodDhcp) {
      // Use the IP address obtained from DHCP.
      // If not obtained, '\0' will be inserted.
      ret = EsfNetworkManagerAccessorGetIFInfo(handle_internal->mode_info,
                                               &ifinfo->normal_mode.dev_ip);
      if (ret != kEsfNetworkManagerResultSuccess) {
        ESF_NETWORK_MANAGER_WARN("Network info get error. ret=%u", ret);
        EsfNetworkManagerIPInfo *ip_info = &ifinfo->normal_mode.dev_ip;
        ip_info->ip[0] = '\0';
        ip_info->subnet_mask[0] = '\0';
        ip_info->gateway[0] = '\0';
        ip_info->dns[0] = '\0';
        ip_info->dns2[0] = '\0';
      }
    }
    ESF_NETWORK_MANAGER_DBG("Network connect information.");
    EsfNetworkManagerShowConnectInfo(handle_internal->mode, ifinfo);
  } while (0);
  if (is_locked) {
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
    }
  }
  ret = EsfNetworkManagerResourceCloseHandle(false, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error.");
    return_result = ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerGetNetstat(
    EsfNetworkManagerHandle handle, const int32_t netstat_buf_size,
    char *netstat_buf) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32
                            ", netstat_buf=%p, netstat_buf_size=%" PRId32,
                            handle, netstat_buf, netstat_buf_size);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX ||
      netstat_buf == NULL || netstat_buf_size <= 0) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32
                            ", netstat_buf=%p, netstat_buf_size=%" PRId32,
                            handle, netstat_buf, netstat_buf_size);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;

  ret = EsfNetworkManagerResourceOpenHandle(false, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }
  ret = EsfNetworkManagerAccessorGetNetstat(netstat_buf_size, netstat_buf);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Netstat get error. ret=%u", ret);
    return_result = ret;
    // fallthrough
  }
  ret = EsfNetworkManagerResourceCloseHandle(false, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error.");
    return_result = ret;
    // fallthrough
  }
  if (return_result == kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_DBG("Netstat get success.\n%s", netstat_buf);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerGetRssi(EsfNetworkManagerHandle handle,
                                                 int8_t *rssi_buf) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32 ", rssi_buf=%p", handle,
                            rssi_buf);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX ||
      rssi_buf == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32 ", rssi_buf=%p",
                            handle, rssi_buf);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;
  bool is_locked = false;

  ret = EsfNetworkManagerResourceOpenHandle(false, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    ret = EsfNetworkManagerResourceLockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      break;
    }
    is_locked = true;
    // An error will occur unless the mode is Normal and
    // Wi-Fi connection has already been started.
    if (!handle_internal->mode_info->is_connect_info_saved ||
        handle_internal->mode != kEsfNetworkManagerModeNormal ||
        (handle_internal->mode_info->connect_status !=
             kEsfNetworkManagerConnectStatusConnected &&
         handle_internal->mode_info->connect_status !=
             kEsfNetworkManagerConnectStatusConnecting) ||
        handle_internal->mode_info->connect_info.normal_mode.netif_kind !=
            kEsfNetworkManagerNetifKindWiFi) {
      ESF_NETWORK_MANAGER_ERR(
          "Not normal mode or wifi connection not started.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    if (handle_internal->mode_info
            ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] == NULL) {
      ESF_NETWORK_MANAGER_ERR("Interface wifi Pl system info not found.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInternalError;
      break;
    }
    char *if_name = handle_internal->mode_info
                        ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi]
                        ->if_name;
    is_locked = false;
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
      break;
    }
    ret = EsfNetworkManagerAccessorGetRssi(if_name, rssi_buf);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Network get rssi error. ret=%u", ret);
      return_result = ret;
      break;
    }
    ESF_NETWORK_MANAGER_DBG("Get rssi success. %d", *rssi_buf);
  } while (0);
  if (is_locked) {
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      return_result = ret;
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    }
  }
  ret = EsfNetworkManagerResourceCloseHandle(false, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    return_result = ret;
    ESF_NETWORK_MANAGER_ERR("Handle close error.");
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerRegisterCallback(
    EsfNetworkManagerHandle handle,
    EsfNetworkManagerNotifyInfoCallback notify_callback, void *private_data) {
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32 ", notify_callback=%p",
                            handle, notify_callback);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX ||
      notify_callback == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32
                            ", notify_callback=%p",
                            handle, notify_callback);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;

  ret = EsfNetworkManagerResourceOpenHandle(false, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }

  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  do {
    ret = EsfNetworkManagerResourceLockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      break;
    }
    if (handle_internal->mode_info->callback != NULL) {
      ESF_NETWORK_MANAGER_ERR("The callback function is already registered.");
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      return_result = kEsfNetworkManagerResultAlreadyCallbackRegistered;
    } else {
      handle_internal->mode_info->callback = notify_callback;
      handle_internal->mode_info->callback_private_data = private_data;
      // Notify the latest current status.
      EsfNetworkManagerAccessorCallCallback(handle_internal->mode,
                                            handle_internal->mode_info);
      ESF_NETWORK_MANAGER_DBG("Register callback success. %p %p",
                              notify_callback, private_data);
    }
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
      break;
    }
  } while (0);
#else
  if (handle_internal->mode_info->callback != NULL) {
    ESF_NETWORK_MANAGER_ERR("The callback function is already registered.");
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
    return_result = kEsfNetworkManagerResultAlreadyCallbackRegistered;
  } else {
    handle_internal->mode_info->callback = notify_callback;
    handle_internal->mode_info->callback_private_data = private_data;
    // Notify the latest current status.
    EsfNetworkManagerAccessorCallCallback(handle_internal->mode,
                                          handle_internal->mode_info);
    ESF_NETWORK_MANAGER_DBG("Register callback success. %p %p", notify_callback,
                            private_data);
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ret = EsfNetworkManagerResourceCloseHandle(false, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error.");
    return_result = ret;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerUnregisterCallback(
    EsfNetworkManagerHandle handle) {
  ESF_NETWORK_MANAGER_TRACE("START handle=%" PRId32, handle);
  if (handle < 0 || handle >= ESF_NETWORK_MANAGER_HANDLE_MAX) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle=%" PRId32, handle);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerHandleInternal *handle_internal = NULL;

  ret = EsfNetworkManagerResourceOpenHandle(false, handle, &handle_internal);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle open error. ret=%u", ret);
    return ret;
  }

  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  do {
    ret = EsfNetworkManagerResourceLockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource lock error.");
      return_result = ret;
      break;
    }
    if (handle_internal->mode_info->callback == NULL) {
      ESF_NETWORK_MANAGER_ERR("The callback function is already Unregistered.");
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      return_result = kEsfNetworkManagerResultAlreadyCallbackUnregistered;
    } else {
      ESF_NETWORK_MANAGER_DBG(
          "Unregister callback success. %p %p",
          handle_internal->mode_info->callback,
          handle_internal->mode_info->callback_private_data);
      handle_internal->mode_info->callback = NULL;
      handle_internal->mode_info->callback_private_data = NULL;
    }
    ret = EsfNetworkManagerResourceUnlockResource();
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
      return_result = ret;
      break;
    }
  } while (0);
#else
  if (handle_internal->mode_info->callback == NULL) {
    ESF_NETWORK_MANAGER_ERR("The callback function is already Unregistered.");
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
    return_result = kEsfNetworkManagerResultAlreadyCallbackUnregistered;
  } else {
    ESF_NETWORK_MANAGER_DBG("Unregister callback success. %p %p",
                            handle_internal->mode_info->callback,
                            handle_internal->mode_info->callback_private_data);
    handle_internal->mode_info->callback = NULL;
    handle_internal->mode_info->callback_private_data = NULL;
  }
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

  ret = EsfNetworkManagerResourceCloseHandle(false, handle);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Handle close error.");
    return_result = ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult EsfNetworkManagerSaveParameter(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p", mask, parameter);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask=%p, parameter=%p)", mask,
                            parameter);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult result = EsfNetworkManagerResourceLockResource();
  if (result != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    return result;
  }
  ESF_NETWORK_MANAGER_DBG("Save parameter.");
  EsfNetworkManagerShowParameter(mask, parameter);
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (!EsfNetworkManagerResourceCheckStatus(kEsfNetworkManagerStatusInit)) {
      ESF_NETWORK_MANAGER_ERR("Not initialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    if (!EsfNetworkManagerValidateParameterInternal(mask, parameter, false)) {
      ESF_NETWORK_MANAGER_ERR("Parameter anomaly detection.");
      return_result = kEsfNetworkManagerResultInvalidParameter;
      break;
    }

    result = EsfNetworkManagerParameterStorageManagerAccessorSaveNetwork(
        mask, parameter);
    if (result != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Save error. (result=%u)", result);
      return_result = result;
    } else {
      ESF_NETWORK_MANAGER_DBG("Save parameter success.");
    }
  } while (0);
  result = EsfNetworkManagerResourceUnlockResource();
  if (result != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    return_result = result;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerLoadParameter(
    const EsfNetworkManagerParameterMask *mask,
    EsfNetworkManagerParameter *parameter) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p", mask, parameter);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask=%p, parameter=%p)", mask,
                            parameter);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult result = EsfNetworkManagerResourceLockResource();
  if (result != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    return result;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    if (!EsfNetworkManagerResourceCheckStatus(kEsfNetworkManagerStatusInit)) {
      ESF_NETWORK_MANAGER_ERR("Not initialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultStatusUnexecutable;
      break;
    }
    result = EsfNetworkManagerParameterStorageManagerAccessorLoadNetwork(
        mask, parameter);
    if (result != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Load error. (result=%u)", result);
      return_result = result;
    } else {
      ESF_NETWORK_MANAGER_DBG("Load parameter success.");
      EsfNetworkManagerShowParameter(mask, parameter);
    }
  } while (0);
  result = EsfNetworkManagerResourceUnlockResource();
  if (result != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    return_result = result;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerSaveVariantParameter(
    EsfNetworkManagerParameterType parameter_type,
    const EsfNetworkManagerVariantParameterMask *mask,
    const EsfNetworkManagerVariantParameter *parameter) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START parameter_type=%u mask=%p parameter=%p",
                            parameter_type, mask, parameter);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask=%p, parameter=%p)", mask,
                            parameter);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  ESF_NETWORK_MANAGER_DBG("Save variant parameter.");
  do {
    EsfNetworkManagerParameterMask *mask_work = NULL;
    EsfNetworkManagerParameter *parameter_work = NULL;
    ret =
        EsfNetworkManagerResourceGetParameterWork(&mask_work, &parameter_work);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource get parameter error.");
      return_result = ret;
      break;
    }
    memset(mask_work, 0, sizeof(*mask_work));
    memset(parameter_work, 0, sizeof(*parameter_work));
    switch (parameter_type) {
      case kEsfNetworkManagerParameterTypeNormal:
        mask_work->normal_mode = mask->normal_mode;
        parameter_work->normal_mode = parameter->normal_mode;
        break;
      case kEsfNetworkManagerParameterTypeAccessPoint:
        mask_work->accesspoint_mode = mask->accesspoint_mode;
        parameter_work->accesspoint_mode = parameter->accesspoint_mode;
        break;
      case kEsfNetworkManagerParameterTypeProxy:
        mask_work->proxy = mask->proxy;
        parameter_work->proxy = parameter->proxy;
        break;
      default:
        ESF_NETWORK_MANAGER_ERR("Parameter error. parameter_type=%" PRId32,
                                parameter_type);
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
        return_result = kEsfNetworkManagerResultInvalidParameter;
        break;
    }
    if (return_result != kEsfNetworkManagerResultSuccess) {
      break;
    }
    EsfNetworkManagerShowParameter(mask_work, parameter_work);
    if (!EsfNetworkManagerValidateParameterInternal(mask_work, parameter_work,
                                                    false)) {
      ESF_NETWORK_MANAGER_ERR("Parameter anomaly detection.");
      return_result = kEsfNetworkManagerResultInvalidParameter;
      break;
    }
    ret = EsfNetworkManagerParameterStorageManagerAccessorSaveNetwork(
        mask_work, parameter_work);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Save error. (ret=%u)", ret);
      return_result = ret;
    } else {
      ESF_NETWORK_MANAGER_DBG("Save parameter success.");
    }
  } while (0);
  ret = EsfNetworkManagerResourceUnlockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    return_result = ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

EsfNetworkManagerResult EsfNetworkManagerLoadVariantParameter(
    EsfNetworkManagerParameterType parameter_type,
    const EsfNetworkManagerVariantParameterMask *mask,
    EsfNetworkManagerVariantParameter *parameter) {
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
  ESF_NETWORK_MANAGER_TRACE("START parameter_type=%u mask=%p parameter=%p",
                            parameter_type, mask, parameter);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask=%p, parameter=%p)", mask,
                            parameter);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.");
    return ret;
  }
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  ESF_NETWORK_MANAGER_DBG("Load variant parameter.");
  do {
    EsfNetworkManagerParameterMask *mask_work = NULL;
    EsfNetworkManagerParameter *parameter_work = NULL;
    ret =
        EsfNetworkManagerResourceGetParameterWork(&mask_work, &parameter_work);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Resource get parameter error.");
      return_result = ret;
      break;
    }
    memset(mask_work, 0, sizeof(*mask_work));
    memset(parameter_work, 0, sizeof(*parameter_work));
    void *mask_work_target, *parameter_work_target, *parameter_target;
    const void *mask_target;
    size_t mask_size, parameter_size;
    switch (parameter_type) {
      case kEsfNetworkManagerParameterTypeNormal:
        mask_work_target = &mask_work->normal_mode;
        parameter_work_target = &parameter_work->normal_mode;
        mask_target = &mask->normal_mode;
        parameter_target = &parameter->normal_mode;
        mask_size = sizeof(mask->normal_mode);
        parameter_size = sizeof(parameter->normal_mode);
        break;
      case kEsfNetworkManagerParameterTypeAccessPoint:
        mask_work_target = &mask_work->accesspoint_mode;
        parameter_work_target = &parameter_work->accesspoint_mode;
        mask_target = &mask->accesspoint_mode;
        parameter_target = &parameter->accesspoint_mode;
        mask_size = sizeof(mask->accesspoint_mode);
        parameter_size = sizeof(parameter->accesspoint_mode);
        break;
      case kEsfNetworkManagerParameterTypeProxy:
        mask_work_target = &mask_work->proxy;
        parameter_work_target = &parameter_work->proxy;
        mask_target = &mask->proxy;
        parameter_target = &parameter->proxy;
        mask_size = sizeof(mask->proxy);
        parameter_size = sizeof(parameter->proxy);
        break;
      default:
        ESF_NETWORK_MANAGER_ERR("Parameter error. parameter_type=%" PRId32,
                                parameter_type);
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
        return_result = kEsfNetworkManagerResultInvalidParameter;
        break;
    }
    if (return_result != kEsfNetworkManagerResultSuccess) {
      break;
    }
    memcpy(mask_work_target, mask_target, mask_size);
    ret = EsfNetworkManagerParameterStorageManagerAccessorLoadNetwork(
        mask_work, parameter_work);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Load error. (ret=%u)", ret);
      return_result = ret;
    } else {
      ESF_NETWORK_MANAGER_DBG("Load parameter success.");
      EsfNetworkManagerShowParameter(mask_work, parameter_work);
    }
    memcpy(parameter_target, parameter_work_target, parameter_size);
  } while (0);
  ret = EsfNetworkManagerResourceUnlockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
    return_result = ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
#else
  return kEsfNetworkManagerResultSuccess;
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
}

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
// """Checks the sanity of the IP address string.

// Check whether conversion is possible with inet_pton.

// Args:
//     af (int): IP version specification. (AF_INET, AF_INET6)
//     addr (const char *): IP Address String.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateParameterInternalIpaddress(
    int af, const char *addr) {
  ESF_NETWORK_MANAGER_TRACE("START addr=%p", addr);
  if (addr == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. addr=%p", addr);
    return false;
  }
  int ret = 0;
  unsigned char buf[sizeof(struct in6_addr)];
  ret = inet_pton(af, addr, buf);
  if (ret != 1) {
    ESF_NETWORK_MANAGER_ERR("IP address validate error. ip=%s ret=%d errno=%d",
                            addr, ret, errno);
    return false;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIp IP parameter sanity check function.

// Normal mode DevIp IP parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpIp(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip.ip) {
    if (!is_connect) {
      size_t len = strnlen(parameter->normal_mode.dev_ip.ip,
                           sizeof(parameter->normal_mode.dev_ip.ip));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->normal_mode.dev_ip.ip)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip IP address validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIp mask parameter sanity check function.

// Normal mode DevIp mask parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpMask(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip.subnet_mask) {
    if (!is_connect) {
      size_t len = strnlen(parameter->normal_mode.dev_ip.subnet_mask,
                           sizeof(parameter->normal_mode.dev_ip.subnet_mask));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->normal_mode.dev_ip.subnet_mask)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip subnetmask validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIp gateway parameter sanity check function.

// Normal mode DevIp gateway parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpGateway(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip.gateway) {
    if (!is_connect) {
      size_t len = strnlen(parameter->normal_mode.dev_ip.gateway,
                           sizeof(parameter->normal_mode.dev_ip.gateway));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->normal_mode.dev_ip.gateway)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip gateway validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIp dns parameter sanity check function.

// Normal mode DevIp dns parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpDns(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip.dns) {
    (void)is_connect;
    // Dns is optional
    // Allow NULL character when connecting
    size_t len = strnlen(parameter->normal_mode.dev_ip.dns,
                          sizeof(parameter->normal_mode.dev_ip.dns));
    if (len == 0) {
      // Delete data, OK.
      return true;
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->normal_mode.dev_ip.dns)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip dns validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIp dns2 parameter sanity check function.

// Normal mode DevIp dns2 parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpDns2(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip.dns2) {
    (void)is_connect;
    // Dns2 is optional
    // Allow NULL character when connecting
    size_t len = strnlen(parameter->normal_mode.dev_ip.dns2,
                         sizeof(parameter->normal_mode.dev_ip.dns2));
    if (len == 0) {
      // Delete data, OK.
      return true;
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->normal_mode.dev_ip.dns2)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip dns2 validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIpv6 ip parameter sanity check function.

// Normal mode DevIpv6 ip parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpv6Ip(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip_v6.ip) {
    if (!is_connect) {
      size_t len = strnlen(parameter->normal_mode.dev_ip_v6.ip,
                           sizeof(parameter->normal_mode.dev_ip_v6.ip));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET6, parameter->normal_mode.dev_ip_v6.ip)) {
      ESF_NETWORK_MANAGER_ERR(
          "Normal mode dev_ip_v6 IP address validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIpv6 mask parameter sanity check function.

// Normal mode DevIpv6 mask parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpv6Mask(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip_v6.subnet_mask) {
    if (!is_connect) {
      size_t len =
          strnlen(parameter->normal_mode.dev_ip_v6.subnet_mask,
                  sizeof(parameter->normal_mode.dev_ip_v6.subnet_mask));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET6, parameter->normal_mode.dev_ip_v6.subnet_mask)) {
      ESF_NETWORK_MANAGER_ERR(
          "Normal mode dev_ip_v6 subnetmask validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIpv6 gateway parameter sanity check function.

// Normal mode DevIpv6 gateway parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpv6Gateway(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip_v6.gateway) {
    if (!is_connect) {
      size_t len = strnlen(parameter->normal_mode.dev_ip_v6.gateway,
                           sizeof(parameter->normal_mode.dev_ip_v6.gateway));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET6, parameter->normal_mode.dev_ip_v6.gateway)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip_v6 gateway validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode DevIpv6 dns parameter sanity check function.

// Normal mode DevIpv6 dns parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterDevIpv6Dns(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.dev_ip_v6.dns) {
    if (!is_connect) {
      size_t len = strnlen(parameter->normal_mode.dev_ip_v6.dns,
                           sizeof(parameter->normal_mode.dev_ip_v6.dns));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET6, parameter->normal_mode.dev_ip_v6.dns)) {
      ESF_NETWORK_MANAGER_ERR("Normal mode dev_ip_v6 dns validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode WifiSta ssid parameter sanity check function.

// Normal mode WifiSta ssid parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterWifiStaSsid(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.wifi_sta.ssid) {
    size_t len = strnlen(parameter->normal_mode.wifi_sta.ssid,
                         sizeof(parameter->normal_mode.wifi_sta.ssid));
    if (is_connect) {
      if (len == 0 || len == sizeof(parameter->normal_mode.wifi_sta.ssid)) {
        ESF_NETWORK_MANAGER_ERR(
            "Normal mode wifi_sta ssid validate error. len=%zu, range=[1, %zu]",
            len, sizeof(parameter->normal_mode.wifi_sta.ssid) - 1);
        return false;
      }
    } else {
      if (len == sizeof(parameter->normal_mode.wifi_sta.ssid)) {
        ESF_NETWORK_MANAGER_ERR(
            "Normal mode wifi_sta ssid validate error. len=%zu, range=[0, %zu]",
            len, sizeof(parameter->normal_mode.wifi_sta.ssid) - 1);
        return false;
      }
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode WifiSta password parameter sanity check function.

// Normal mode WifiSta password parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterWifiStaPassword(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.wifi_sta.password) {
    size_t len = strnlen(parameter->normal_mode.wifi_sta.password,
                         sizeof(parameter->normal_mode.wifi_sta.password));
    if (len == sizeof(parameter->normal_mode.wifi_sta.password)) {
      ESF_NETWORK_MANAGER_ERR(
          "Normal mode wifi_sta password validate error. len=%zu, range=[0, "
          "%zu]",
          len, sizeof(parameter->normal_mode.wifi_sta.password) - 1);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode WifiSta encryption parameter sanity check function.

// Normal mode WifiSta encryption parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterWifiStaEncryption(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.wifi_sta.encryption) {
    if (parameter->normal_mode.wifi_sta.encryption < 0 ||
        parameter->normal_mode.wifi_sta.encryption > 2) {
      ESF_NETWORK_MANAGER_ERR(
          "Normal mode wifi_sta encryption validate error. encryption=%d, "
          "range[0, 2]",
          parameter->normal_mode.wifi_sta.encryption);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode ip_method parameter sanity check function.

// Normal mode ip_method parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterIpMethod(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.ip_method) {
    switch (parameter->normal_mode.ip_method) {
      case kEsfNetworkManagerIpMethodDhcp:
      case kEsfNetworkManagerIpMethodStatic:
        break;
      default:
        ESF_NETWORK_MANAGER_ERR(
            "Normal mode ip_method validate error. ip_method=%d, range[%d, "
            "%d]",
            parameter->normal_mode.ip_method,
            (int)kEsfNetworkManagerIpMethodDhcp,
            (int)kEsfNetworkManagerIpMethodStatic);
        return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Normal mode netif_kind parameter sanity check function.

// Normal mode netif_kind parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterNetifKind(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->normal_mode.netif_kind) {
    switch (parameter->normal_mode.netif_kind) {
      case kEsfNetworkManagerNetifKindWiFi:
      case kEsfNetworkManagerNetifKindEther:
        break;
      default:
        ESF_NETWORK_MANAGER_ERR(
            "Normal mode netif_kind validate error. netif_kind=%d, range[%d, "
            "%d]",
            parameter->normal_mode.netif_kind,
            (int)kEsfNetworkManagerNetifKindWiFi,
            (int)kEsfNetworkManagerNetifKindEther);
        return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode DevIp ip parameter sanity check function.

// Access point mode DevIp ip parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpIp(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.dev_ip.ip) {
    if (!is_connect) {
      size_t len = strnlen(parameter->accesspoint_mode.dev_ip.ip,
                           sizeof(parameter->accesspoint_mode.dev_ip.ip));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->accesspoint_mode.dev_ip.ip)) {
      ESF_NETWORK_MANAGER_ERR(
          "Access point mode dev_ip IP address validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode DevIp mask parameter sanity check function.

// Access point mode DevIp mask parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpMask(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.dev_ip.subnet_mask) {
    if (!is_connect) {
      size_t len =
          strnlen(parameter->accesspoint_mode.dev_ip.subnet_mask,
                  sizeof(parameter->accesspoint_mode.dev_ip.subnet_mask));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->accesspoint_mode.dev_ip.subnet_mask)) {
      ESF_NETWORK_MANAGER_ERR(
          "Access point mode dev_ip subnetmask validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode DevIp gateway parameter sanity check function.

// Access point mode DevIp gateway parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpGateway(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.dev_ip.gateway) {
    if (!is_connect) {
      size_t len = strnlen(parameter->accesspoint_mode.dev_ip.gateway,
                           sizeof(parameter->accesspoint_mode.dev_ip.gateway));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->accesspoint_mode.dev_ip.gateway)) {
      ESF_NETWORK_MANAGER_ERR(
          "Access point mode dev_ip gateway validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode DevIp dns parameter sanity check function.

// Access point mode DevIp dns parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointDevIpDns(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.dev_ip.dns) {
    if (!is_connect) {
      size_t len = strnlen(parameter->accesspoint_mode.dev_ip.dns,
                           sizeof(parameter->accesspoint_mode.dev_ip.dns));
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (!EsfNetworkManagerValidateParameterInternalIpaddress(
            AF_INET, parameter->accesspoint_mode.dev_ip.dns)) {
      ESF_NETWORK_MANAGER_ERR("Access point mode dev_ip dns validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode WifiAp ssid parameter sanity check function.

// Access point mode WifiAp ssid parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointWifiApSsid(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.wifi_ap.ssid) {
    size_t len = strnlen(parameter->accesspoint_mode.wifi_ap.ssid,
                         sizeof(parameter->accesspoint_mode.wifi_ap.ssid));
    if (!is_connect) {
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (len == 0 || len == sizeof(parameter->accesspoint_mode.wifi_ap.ssid)) {
      ESF_NETWORK_MANAGER_ERR("Access point mode wifi_ap ssid validate error.");
      return false;
    }
    for (size_t i = 0; i < len; ++i) {
      if (isgraph(parameter->accesspoint_mode.wifi_ap.ssid[i]) == 0) {
        ESF_NETWORK_MANAGER_ERR(
            "Access point mode wifi_ap ssid validate error.");
        return false;
      }
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode WifiAp password parameter sanity check function.

// Access point mode WifiAp password parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointWifiApPassword(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.wifi_ap.password) {
    size_t len = strnlen(parameter->accesspoint_mode.wifi_ap.password,
                         sizeof(parameter->accesspoint_mode.wifi_ap.password));
    if (!is_connect) {
      if (len == 0) {
        // Delete data, OK.
        return true;
      }
    }
    if (len < 8 ||
        len == sizeof(parameter->accesspoint_mode.wifi_ap.password)) {
      ESF_NETWORK_MANAGER_ERR(
          "Access point mode wifi_ap password validate error.");
      return false;
    }
    if (len == 64) {  // A 64 character hex string
      for (size_t i = 0; i < len; ++i) {
        if (!isxdigit(parameter->accesspoint_mode.wifi_ap.password[i])) {
          ESF_NETWORK_MANAGER_ERR(
              "Access point mode wifi_ap password validate error.");
          return false;
        }
      }
    } else {
      for (size_t i = 0; i < len; ++i) {
        if (isgraph(parameter->accesspoint_mode.wifi_ap.password[i]) == 0) {
          ESF_NETWORK_MANAGER_ERR(
              "Access point mode wifi_ap password validate error.");
          return false;
        }
      }
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Access point mode WifiAp encryption sanity check function.

// Access point mode WifiAp encryption parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterAccessPointWifiApEncryption(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->accesspoint_mode.wifi_ap.encryption) {
    if (parameter->accesspoint_mode.wifi_ap.encryption < 0 ||
        parameter->accesspoint_mode.wifi_ap.encryption > 2) {
      ESF_NETWORK_MANAGER_ERR(
          "Access point mode wifi_ap encryption validate error.");
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Proxy Url sanity check function.

// Proxy Url sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterProxyUrl(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->proxy.url) {
    size_t len = strnlen(parameter->proxy.url, sizeof(parameter->proxy.url));
    if (len == sizeof(parameter->proxy.url)) {
      ESF_NETWORK_MANAGER_ERR(
          "Proxy Url validate error. len=%zu, range=[0, "
          "%zu]",
          len, sizeof(parameter->proxy.url) - 1);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Proxy port sanity check function.

// Proxy port sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterProxyPort(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->proxy.port) {
    if (parameter->proxy.port < ESF_NETWORK_MANAGER_PORT_MIN ||
        parameter->proxy.port > ESF_NETWORK_MANAGER_PORT_MAX) {
      ESF_NETWORK_MANAGER_ERR(
          "Proxy port validate error. port=%d, range=[%d, %d]",
          parameter->proxy.port, ESF_NETWORK_MANAGER_PORT_MIN,
          ESF_NETWORK_MANAGER_PORT_MAX);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Proxy username sanity check function.

// Proxy username sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterProxyUsername(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->proxy.username) {
    size_t len =
        strnlen(parameter->proxy.username, sizeof(parameter->proxy.username));
    if (len == sizeof(parameter->proxy.username)) {
      ESF_NETWORK_MANAGER_ERR(
          "Proxy username validate error. len=%zu, range=[0, "
          "%zu]",
          len, sizeof(parameter->proxy.username) - 1);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Proxy password sanity check function.

// Proxy password sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateOneParameterProxyPassword(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  (void)is_connect;
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  if (mask->proxy.password) {
    size_t len =
        strnlen(parameter->proxy.password, sizeof(parameter->proxy.password));
    if (len == sizeof(parameter->proxy.password)) {
      ESF_NETWORK_MANAGER_ERR(
          "Proxy password validate error. len=%zu, range=[0, "
          "%zu]",
          len, sizeof(parameter->proxy.password) - 1);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Network parameter sanity check function.

// Network parameter sanity check function.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.
//     is_connect (bool):
//       Identification information indicating whether the data subject to
//       checking is for connection use.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateParameterInternal(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter, bool is_connect) {
  // Individual parameter check function table.
  static const EsfNetworkManagerValidateOneParameter kValidateFuncTable[] = {
      EsfNetworkManagerValidateOneParameterDevIpIp,
      EsfNetworkManagerValidateOneParameterDevIpMask,
      EsfNetworkManagerValidateOneParameterDevIpGateway,
      EsfNetworkManagerValidateOneParameterDevIpDns,
      EsfNetworkManagerValidateOneParameterDevIpDns2,
      EsfNetworkManagerValidateOneParameterDevIpv6Ip,
      EsfNetworkManagerValidateOneParameterDevIpv6Mask,
      EsfNetworkManagerValidateOneParameterDevIpv6Gateway,
      EsfNetworkManagerValidateOneParameterDevIpv6Dns,
      EsfNetworkManagerValidateOneParameterWifiStaSsid,
      EsfNetworkManagerValidateOneParameterWifiStaPassword,
      EsfNetworkManagerValidateOneParameterWifiStaEncryption,
      EsfNetworkManagerValidateOneParameterIpMethod,
      EsfNetworkManagerValidateOneParameterNetifKind,
      EsfNetworkManagerValidateOneParameterAccessPointDevIpIp,
      EsfNetworkManagerValidateOneParameterAccessPointDevIpMask,
      EsfNetworkManagerValidateOneParameterAccessPointDevIpGateway,
      EsfNetworkManagerValidateOneParameterAccessPointDevIpDns,
      EsfNetworkManagerValidateOneParameterAccessPointWifiApSsid,
      EsfNetworkManagerValidateOneParameterAccessPointWifiApPassword,
      EsfNetworkManagerValidateOneParameterAccessPointWifiApEncryption,
      EsfNetworkManagerValidateOneParameterProxyUrl,
      EsfNetworkManagerValidateOneParameterProxyPort,
      EsfNetworkManagerValidateOneParameterProxyUsername,
      EsfNetworkManagerValidateOneParameterProxyPassword,
  };
  ESF_NETWORK_MANAGER_TRACE("START mask=%p parameter=%p is_connect=%d", mask,
                            parameter, is_connect);
  if (mask == NULL || parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. mask=%p parameter=%p", mask,
                            parameter);
    return false;
  }
  for (size_t i = 0;
       i < sizeof(kValidateFuncTable) / sizeof(kValidateFuncTable[0]); ++i) {
    if (!kValidateFuncTable[i](mask, parameter, is_connect)) {
      ESF_NETWORK_MANAGER_ERR("Validate error. index=%zu", i);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM);
      return false;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return true;
}

// """Network parameter sanity check function.

// Transfer the network connection information into a sanity check structure.
// Verify the contents using the sanity check function.

// Args:
//     handle_internal (const EsfNetworkManagerHandleInternal *):
//       Network internal resource information.
//     os_info (const EsfNetworkManagerOSInfo *):
//       Network connection information.

// Returns:
//     The check result is returned.

// Yields:
//     true:Check OK.
//     false:Check NG.

// Note:
// """
static bool EsfNetworkManagerValidateConnectInfo(
    const EsfNetworkManagerHandleInternal *handle_internal,
    const EsfNetworkManagerOSInfo *os_info) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p, os_info=%p",
                            handle_internal, os_info);
  if (handle_internal == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle_internal=%p",
                            handle_internal);
    return false;
  }
  if (os_info == NULL) {
    ESF_NETWORK_MANAGER_DBG("os_info NULL");
    return true;
  }
  EsfNetworkManagerParameterMask *mask =
      (EsfNetworkManagerParameterMask *)calloc(
          1, sizeof(EsfNetworkManagerParameterMask));
  if (mask == NULL) {
    ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return false;
  }
  EsfNetworkManagerParameter *parameter = (EsfNetworkManagerParameter *)calloc(
      1, sizeof(EsfNetworkManagerParameter));
  if (parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    free(mask);
    return false;
  }
  if (handle_internal->mode == kEsfNetworkManagerModeNormal) {
    mask->normal_mode.ip_method = 1;
    parameter->normal_mode.ip_method = os_info->normal_mode.ip_method;
    mask->normal_mode.netif_kind = 1;
    parameter->normal_mode.netif_kind = os_info->normal_mode.netif_kind;
    if (os_info->normal_mode.ip_method == kEsfNetworkManagerIpMethodStatic) {
      mask->normal_mode.dev_ip.ip = 1;
      mask->normal_mode.dev_ip.subnet_mask = 1;
      mask->normal_mode.dev_ip.gateway = 1;
      mask->normal_mode.dev_ip.dns = 1;
      mask->normal_mode.dev_ip.dns2 = 1;
      parameter->normal_mode.dev_ip = os_info->normal_mode.dev_ip;
    }
    if (os_info->normal_mode.netif_kind == kEsfNetworkManagerNetifKindWiFi) {
      mask->normal_mode.wifi_sta.ssid = 1;
      mask->normal_mode.wifi_sta.password = 1;
      mask->normal_mode.wifi_sta.encryption = 1;
      parameter->normal_mode.wifi_sta = os_info->normal_mode.wifi_sta;
    }
  } else {  // AccessPoint mode
    mask->accesspoint_mode.wifi_ap.ssid = 1;
    mask->accesspoint_mode.wifi_ap.password = 1;
    mask->accesspoint_mode.wifi_ap.encryption = 1;
    mask->accesspoint_mode.wifi_ap.channel = 1;
    parameter->accesspoint_mode.wifi_ap = os_info->accesspoint_mode.wifi_ap;
    if (os_info->accesspoint_mode.dev_ip.ip[0] != '\0') {
      mask->accesspoint_mode.dev_ip.ip = 1;
      mask->accesspoint_mode.dev_ip.subnet_mask = 1;
      mask->accesspoint_mode.dev_ip.gateway = 1;
      mask->accesspoint_mode.dev_ip.dns = 1;
      parameter->accesspoint_mode.dev_ip = os_info->accesspoint_mode.dev_ip;
    }
  }
  bool ret = EsfNetworkManagerValidateParameterInternal(mask, parameter, true);
  free(mask);
  free(parameter);
  ESF_NETWORK_MANAGER_TRACE("END");
  return ret;
}

// """Updates internal information when the network is down.

// Updates information for each connection mode and connection interface.
// Checks whether a callback function needs to be called.

// Args:
//     handle_internal (const EsfNetworkManagerHandleInternal *):
//       Network internal resource information.

// Returns:
//     Whether or not to call the callback function.

// Yields:
//     true:Need to call the callback function.
//     false:Don't need to call the callback function.

// Note:
// """
static void EsfNetworkManagerUpdateStopStatus(
    EsfNetworkManagerHandleInternal *handle_internal) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p", handle_internal);
  if (handle_internal == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle_internal=%p",
                            handle_internal);
    return;
  }
  ESF_NETWORK_MANAGER_DBG("Update status mode=%d", handle_internal->mode);
  switch (handle_internal->mode) {
    case kEsfNetworkManagerModeNormal:
      switch (handle_internal->mode_info->connect_info.normal_mode.netif_kind) {
        case kEsfNetworkManagerNetifKindWiFi:
          if (handle_internal->mode_info->connect_status ==
              kEsfNetworkManagerConnectStatusConnected) {
            // In the case of a disconnection from a WiFi connection,
            // there is no PL event trigger,
            // so you need to call the state change callback here.
            handle_internal->mode_info->notify_info =
                kEsfNetworkManagerNotifyInfoDisconnected;
            EsfNetworkManagerAccessorCallCallback(handle_internal->mode,
                                                  handle_internal->mode_info);
          }
          handle_internal->mode_info->connect_status =
              kEsfNetworkManagerConnectStatusDisconnected;
          break;
        case kEsfNetworkManagerNetifKindEther:
          if (handle_internal->mode_info->status_info.is_if_up) {
            handle_internal->mode_info->connect_status =
                kEsfNetworkManagerConnectStatusDisconnecting;
          } else {
            handle_internal->mode_info->connect_status =
                kEsfNetworkManagerConnectStatusDisconnected;
          }
          break;
        default:
          ESF_NETWORK_MANAGER_ERR(
              "Invalid netif_kind. netif_kind=%d",
              handle_internal->mode_info->connect_info.normal_mode.netif_kind);
          break;
      }
      break;
    case kEsfNetworkManagerModeAccessPoint:
      if (handle_internal->mode_info->connect_status ==
          kEsfNetworkManagerConnectStatusConnected) {
        // In the case of a disconnection from a WiFi connection,
        // there is no PL event trigger,
        // so you need to call the state change callback here.
        handle_internal->mode_info->notify_info =
            kEsfNetworkManagerNotifyInfoDisconnected;
        EsfNetworkManagerAccessorCallCallback(handle_internal->mode,
                                              handle_internal->mode_info);
      }
      handle_internal->mode_info->connect_status =
          kEsfNetworkManagerConnectStatusDisconnected;
      break;
    default:
      ESF_NETWORK_MANAGER_ERR("Invalid mode. mode=%u", handle_internal->mode);
      break;
  }
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          handle_internal->mode_info->connect_status,
                          handle_internal->mode_info->notify_info);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

// """Gets the interface type used by GetIFStatus.

// Gets the interface type used by GetIFStatus.

// Args:
//     handle_internal (const EsfNetworkManagerHandleInternal *):
//       Network internal resource information.
//     interface_kind (EsfNetworkManagerInterfaceKind *):
//       Interface type.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerGetInterfaceForGetIFStatus(
    const EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerInterfaceKind *interface_kind) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p, interface_kind=%p",
                            handle_internal, interface_kind);
  if (handle_internal == NULL || interface_kind == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "Parameter error. handle_internal=%p, interface_kind=%p",
        handle_internal, interface_kind);
    return kEsfNetworkManagerResultInternalError;
  }
  *interface_kind = kEsfNetworkManagerInterfaceKindWifi;
  if (handle_internal->mode == kEsfNetworkManagerModeNormal) {
    if (handle_internal->mode_info->is_connect_info_saved) {
      if (handle_internal->mode_info->connect_info.normal_mode.netif_kind ==
          kEsfNetworkManagerNetifKindEther) {
        *interface_kind = kEsfNetworkManagerInterfaceKindEther;
      }
    } else {
      if (handle_internal->mode_info
              ->hal_system_info[kEsfNetworkManagerInterfaceKindWifi] == NULL) {
        *interface_kind = kEsfNetworkManagerInterfaceKindEther;
      }
    }
  }
  if (handle_internal->mode_info->hal_system_info[*interface_kind] == NULL) {
    ESF_NETWORK_MANAGER_ERR("Interface %u Pl system info not found.",
                            *interface_kind);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_TRACE("END interface_kind=%d", *interface_kind);
  return kEsfNetworkManagerResultSuccess;
}

// """Reads network connection information from ParameterStorageManager.

// Reads network connection information from ParameterStorageManager.

// Args:
//     handle_internal (const EsfNetworkManagerHandleInternal *):
//       Network internal resource information.
//     os_info (EsfNetworkManagerOSInfo *): Network connection information.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultExternalError:
//       Error response from ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerLoadConnectInfo(
    const EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerOSInfo *os_info) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p, os_info=%p",
                            handle_internal, os_info);
  if (handle_internal == NULL) {
    ESF_NETWORK_MANAGER_ERR("Parameter error. handle_internal=%p",
                            handle_internal);
    return kEsfNetworkManagerResultInternalError;
  }
  if (os_info == NULL) {
    ESF_NETWORK_MANAGER_ERR("os_info NULL");
    return kEsfNetworkManagerResultInternalError;
  }
  EsfNetworkManagerParameterMask *mask =
      (EsfNetworkManagerParameterMask *)calloc(
          1, sizeof(EsfNetworkManagerParameterMask));
  if (mask == NULL) {
    ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  EsfNetworkManagerParameter *parameter = (EsfNetworkManagerParameter *)calloc(
      1, sizeof(EsfNetworkManagerParameter));
  if (parameter == NULL) {
    ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    free(mask);
    return kEsfNetworkManagerResultInternalError;
  }
  if (handle_internal->mode == kEsfNetworkManagerModeNormal) {
    mask->normal_mode.ip_method = 1;
    mask->normal_mode.netif_kind = 1;
    mask->normal_mode.dev_ip.ip = 1;
    mask->normal_mode.dev_ip.subnet_mask = 1;
    mask->normal_mode.dev_ip.gateway = 1;
    mask->normal_mode.dev_ip.dns = 1;
    mask->normal_mode.dev_ip.dns2 = 1;
    mask->normal_mode.wifi_sta.ssid = 1;
    mask->normal_mode.wifi_sta.password = 1;
    mask->normal_mode.wifi_sta.encryption = 1;
  } else {  // AccessPoint mode
    mask->accesspoint_mode.wifi_ap.ssid = 1;
    mask->accesspoint_mode.wifi_ap.password = 1;
    mask->accesspoint_mode.wifi_ap.encryption = 1;
    mask->accesspoint_mode.wifi_ap.channel = 1;
    mask->accesspoint_mode.dev_ip.ip = 1;
    mask->accesspoint_mode.dev_ip.subnet_mask = 1;
    mask->accesspoint_mode.dev_ip.gateway = 1;
    mask->accesspoint_mode.dev_ip.dns = 1;
  }
  EsfNetworkManagerResult load_result =
      EsfNetworkManagerParameterStorageManagerAccessorLoadNetwork(mask,
                                                                  parameter);
  if (load_result == kEsfNetworkManagerResultSuccess) {
    if (handle_internal->mode == kEsfNetworkManagerModeNormal) {
      os_info->normal_mode = parameter->normal_mode;
    } else {  // AccessPoint mode
      os_info->accesspoint_mode = parameter->accesspoint_mode;
    }
  }
  free(mask);
  free(parameter);
  ESF_NETWORK_MANAGER_TRACE("END");
  return load_result;
}

// """Get connection initiation information.

// Get connection initiation information.
// Gets connection initiation information and check its validity.

// Args:
//     handle_internal (const EsfNetworkManagerHandleInternal *):
//       Network internal resource information.
//     start_type (EsfNetworkManagerStartType):
//       Specify the network connection initiation type.
//     os_info (EsfNetworkManagerOSInfo *):
//       Network connection information.
//     start_connection (EsfNetworkManagerOSInfo **):
//       Sets the obtained connection initiation information.
//     need_free (bool *):
//       A flag indicating whether start_connection needs to be released.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultExternalError:
//       Error response from ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultNoConnectInfo:
//       Network connection information does not exist.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerGetStartConnectInfo(
    const EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerStartType start_type, EsfNetworkManagerOSInfo *os_info,
    EsfNetworkManagerOSInfo **start_connection, bool *need_free) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p, start_type=%" PRId32
                            " os_info=%p start_connection=%p need_free=%p",
                            handle_internal, start_type, os_info,
                            start_connection, need_free);
  if (handle_internal == NULL || start_connection == NULL ||
      need_free == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "Parameter error. handle_internal=%p "
        "start_connection=%p need_free=%p",
        handle_internal, start_connection, need_free);
    return kEsfNetworkManagerResultInternalError;
  }

  *need_free = false;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerOSInfo *work_connection = NULL;
  switch (start_type) {
    case kEsfNetworkManagerStartTypeFuncParameter:
      if (os_info == NULL) {
        ESF_NETWORK_MANAGER_ERR("Parameter error. os_info=%p", os_info);
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
        return_result = kEsfNetworkManagerResultInvalidParameter;
      } else {
        work_connection = os_info;
      }
      break;
    case kEsfNetworkManagerStartTypeSaveParameter:
      work_connection =
          (EsfNetworkManagerOSInfo *)calloc(1, sizeof(EsfNetworkManagerOSInfo));
      if (work_connection == NULL) {
        ESF_NETWORK_MANAGER_ERR("Memory allocate error.");
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
        return_result = kEsfNetworkManagerResultInternalError;
        break;
      }
      *need_free = true;
      ret = EsfNetworkManagerLoadConnectInfo(handle_internal, work_connection);
      if (ret != kEsfNetworkManagerResultSuccess) {
        ESF_NETWORK_MANAGER_ERR("Connection infomation load error.");
        return_result = ret;
      }
      break;
    case kEsfNetworkManagerStartTypeLastStartSuccessParameter:
      if (!handle_internal->mode_info->is_connect_info_saved) {
        ESF_NETWORK_MANAGER_ERR("No connection information available.");
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
        return_result = kEsfNetworkManagerResultNoConnectInfo;
        break;
      }
      work_connection = &handle_internal->mode_info->connect_info;
      break;
    default:
      ESF_NETWORK_MANAGER_ERR("Parameter error. start_type=%" PRId32,
                              start_type);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      return_result = kEsfNetworkManagerResultInvalidParameter;
      break;
  }
  if (return_result == kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_DBG("Network start connect information.");
    EsfNetworkManagerShowConnectInfo(handle_internal->mode, work_connection);
    bool ret_bool =
        EsfNetworkManagerValidateConnectInfo(handle_internal, work_connection);
    if (!ret_bool) {
      ESF_NETWORK_MANAGER_ERR("Connection parameter error.");
      return_result = kEsfNetworkManagerResultInvalidParameter;
    } else {
      *start_connection = work_connection;
    }
  }
  if (return_result != kEsfNetworkManagerResultSuccess && *need_free == true) {
    free(work_connection);
    *need_free = false;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

// """Connection pre-initiation processing.

// Obtains connection information.
// Clears resources before starting a connection.

// Args:
//     handle_internal (EsfNetworkManagerHandleInternal *):
//       Network internal resource information.
//     start_type (EsfNetworkManagerStartType):
//       Specify the network connection initiation type.
//     os_info (EsfNetworkManagerOSInfo *):
//       Network connection information.
//     start_connection (EsfNetworkManagerOSInfo **):
//       Sets the obtained connection initiation information.
//     need_free (bool *):
//       A flag indicating whether start_connection needs to be released.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultStatusAlreadyRunning:
//       Network connection already started.
//     kEsfNetworkManagerResultExternalError:
//       Error response from ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultNoConnectInfo:
//       Network connection information does not exist.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerPreStart(
    EsfNetworkManagerHandleInternal *handle_internal,
    EsfNetworkManagerStartType start_type, EsfNetworkManagerOSInfo *os_info,
    EsfNetworkManagerOSInfo **start_connection, bool *need_free) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p start_type=%" PRId32
                            " os_info=%p start_connection=%p need_free=%p",
                            handle_internal, start_type, os_info,
                            start_connection, need_free);
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;

  if (handle_internal == NULL || start_connection == NULL ||
      need_free == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "Parameter error. handle_internal=%p "
        "start_connection=%p need_free=%p",
        handle_internal, start_connection, need_free);
    return kEsfNetworkManagerResultInternalError;
  }
  do {
    if (handle_internal->mode_info->connect_status !=
        kEsfNetworkManagerConnectStatusDisconnected) {
      ESF_NETWORK_MANAGER_WARN("Connection status is not disconnected.");
      ESF_NETWORK_MANAGER_ELOG_WARN(
          ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR_WARN);
      return_result = kEsfNetworkManagerResultStatusAlreadyRunning;
      break;
    }

    ret = EsfNetworkManagerGetStartConnectInfo(
        handle_internal, start_type, os_info, start_connection, need_free);

    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Get start connection information error.");
      return_result = ret;
      break;
    }

    // Clear Resource data
    handle_internal->mode_info->connect_status =
        kEsfNetworkManagerConnectStatusDisconnected;
    memset(&handle_internal->mode_info->ip_info, 0,
           sizeof(handle_internal->mode_info->ip_info));
    handle_internal->mode_info->ap_connected_count = 0;
    handle_internal->mode_info->status_info.is_if_up = false;
    handle_internal->mode_info->status_info.is_link_up = false;
    handle_internal->mode_info->notify_info =
        kEsfNetworkManagerNotifyInfoDisconnected;
  } while (0);
  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

// """Processing after connection initiation.

// Processing after connection initiation.
// Updates resources after connection is started.

// Args:
//     handle_internal (EsfNetworkManagerHandleInternal *):
//       Network internal resource information.
//     start_connection (const EsfNetworkManagerOSInfo **):
//       Connection start information.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAfterStart(
    EsfNetworkManagerHandleInternal *handle_internal,
    const EsfNetworkManagerOSInfo *start_connection) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p start_connection=%p",
                            handle_internal, start_connection);
  if (handle_internal == NULL || start_connection == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "Parameter error. handle_internal=%p "
        "start_connection=%p",
        handle_internal, start_connection);
    return kEsfNetworkManagerResultInternalError;
  }
  handle_internal->mode_info->connect_info = *start_connection;
  handle_internal->mode_info->is_connect_info_saved = true;
  handle_internal->mode_info->connect_status =
      kEsfNetworkManagerConnectStatusConnecting;
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          handle_internal->mode_info->connect_status,
                          handle_internal->mode_info->notify_info);
  if (handle_internal->mode_info->connect_info.normal_mode.netif_kind ==
      kEsfNetworkManagerNetifKindWiFi) {
    handle_internal->mode_info->status_info.is_if_up = true;
  }

  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

// """Show connection information.

// Show connection information.

// Args:
//     mode (EsfNetworkManagerMode):
//       Network connection mode.
//     os_info (const EsfNetworkManagerOSInfo *):
//       Connection information.

// Note:
// """
static void EsfNetworkManagerShowConnectInfo(
    EsfNetworkManagerMode mode, const EsfNetworkManagerOSInfo *os_info) {
  ESF_NETWORK_MANAGER_TRACE("START");
  if (os_info == NULL) {
    ESF_NETWORK_MANAGER_DBG("os_info NULL");
    return;
  }
  if (mode == kEsfNetworkManagerModeNormal) {
    ESF_NETWORK_MANAGER_DBG("  NORMAL MODE");
    ESF_NETWORK_MANAGER_DBG("    dev_ip.ip         :%s",
                            os_info->normal_mode.dev_ip.ip);
    ESF_NETWORK_MANAGER_DBG("    dev_ip.subnet_mask:%s",
                            os_info->normal_mode.dev_ip.subnet_mask);
    ESF_NETWORK_MANAGER_DBG("    dev_ip.gateway    :%s",
                            os_info->normal_mode.dev_ip.gateway);
    ESF_NETWORK_MANAGER_DBG("    dev_ip.dns        :%s",
                            os_info->normal_mode.dev_ip.dns);
    ESF_NETWORK_MANAGER_DBG("    dev_ip.dns2       :%s",
                            os_info->normal_mode.dev_ip.dns2);
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.ip      :%s",
                            os_info->normal_mode.dev_ip_v6.ip);
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.subnet_mask:%s",
                            os_info->normal_mode.dev_ip_v6.subnet_mask);
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.gateway :%s",
                            os_info->normal_mode.dev_ip_v6.gateway);
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.dns     :%s",
                            os_info->normal_mode.dev_ip_v6.dns);
    ESF_NETWORK_MANAGER_DBG("    ip_method         :%d",
                            os_info->normal_mode.ip_method);
    ESF_NETWORK_MANAGER_DBG("    netif_kind        :%d",
                            os_info->normal_mode.netif_kind);
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.ssid     :%s",
                            os_info->normal_mode.wifi_sta.ssid);
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.password :%s",
                            os_info->normal_mode.wifi_sta.password);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.ssid     :Secret - Not display");
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.password :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.encryption:%d",
                            os_info->normal_mode.wifi_sta.encryption);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

// """Show flash access parameter.

// Show flash access parameter.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameter *):
//       A structure containing the parameters to be checked.

// Note:
// """
static void EsfNetworkManagerShowParameter(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *parameter) {
  ESF_NETWORK_MANAGER_TRACE("START");
  ESF_NETWORK_MANAGER_DBG("  NORMAL MODE");
  if (mask->normal_mode.dev_ip.ip) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.ip         :%s",
                            parameter->normal_mode.dev_ip.ip);
  }
  if (mask->normal_mode.dev_ip.subnet_mask) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.subnet_mask:%s",
                            parameter->normal_mode.dev_ip.subnet_mask);
  }
  if (mask->normal_mode.dev_ip.gateway) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.gateway    :%s",
                            parameter->normal_mode.dev_ip.gateway);
  }
  if (mask->normal_mode.dev_ip.dns) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.dns        :%s",
                            parameter->normal_mode.dev_ip.dns);
  }
  if (mask->normal_mode.dev_ip.dns2) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.dns2       :%s",
                            parameter->normal_mode.dev_ip.dns2);
  }
  if (mask->normal_mode.dev_ip_v6.ip) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.ip      :%s",
                            parameter->normal_mode.dev_ip_v6.ip);
  }
  if (mask->normal_mode.dev_ip_v6.subnet_mask) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.subnet_mask:%s",
                            parameter->normal_mode.dev_ip_v6.subnet_mask);
  }
  if (mask->normal_mode.dev_ip_v6.gateway) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.gateway :%s",
                            parameter->normal_mode.dev_ip_v6.gateway);
  }
  if (mask->normal_mode.dev_ip_v6.dns) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.dns     :%s",
                            parameter->normal_mode.dev_ip_v6.dns);
  }
  if (mask->normal_mode.ip_method) {
    ESF_NETWORK_MANAGER_DBG("    ip_method         :%d",
                            parameter->normal_mode.ip_method);
  }
  if (mask->normal_mode.netif_kind) {
    ESF_NETWORK_MANAGER_DBG("    netif_kind        :%d",
                            parameter->normal_mode.netif_kind);
  }
  if (mask->normal_mode.wifi_sta.ssid) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.ssid     :%s",
                            parameter->normal_mode.wifi_sta.ssid);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.ssid     :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  if (mask->normal_mode.wifi_sta.password) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.password :%s",
                            parameter->normal_mode.wifi_sta.password);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.password :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  if (mask->normal_mode.wifi_sta.encryption) {
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.encryption:%d",
                            parameter->normal_mode.wifi_sta.encryption);
  }
  ESF_NETWORK_MANAGER_DBG("  PROXY");
  if (mask->proxy.url) {
    ESF_NETWORK_MANAGER_DBG("    proxy.url         :%s", parameter->proxy.url);
  }
  if (mask->proxy.port) {
    ESF_NETWORK_MANAGER_DBG("    proxy.port        :%d", parameter->proxy.port);
  }
  if (mask->proxy.username) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.username    :%s",
                            parameter->proxy.username);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.username    :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  if (mask->proxy.password) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.password    :%s",
                            parameter->proxy.password);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.password    :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
