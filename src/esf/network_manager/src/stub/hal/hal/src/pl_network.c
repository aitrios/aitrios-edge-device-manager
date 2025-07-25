/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// self header.
#include "hal/include/pl_network.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include "hal/include/pl.h"
#include "netutils/dhcpc.h"
#include "netutils/netlib.h"

#define ETHER_IF_NAME "wlan0"

static PlNetworkSystemInfo *system_info = NULL;

typedef struct {
  char if_name[PL_NETWORK_IFNAME_LEN];
  PlNetworkEventHandler handler;
  void *private_data;
} PlNetworkStubEventHandlerSave;

static PlNetworkStubEventHandlerSave *handler_save = NULL;

PlErrCode PlNetworkGetSystemInfoStub(uint32_t *info_total_num,
                                     PlNetworkSystemInfo **infos) {
  if (system_info == NULL) {
    printf("system_info is NULL\n");
    return kPlErrCodeError;
  }
  *info_total_num = kPlNetworkIfTypeStubMax;
  *infos = (PlNetworkSystemInfo *)system_info;
  return kPlErrCodeOk;
}

PlErrCode PlNetworkSetConfigStub(const char *if_name,
                                 const PlNetworkConfig *config) {
  printf("PlNetworkSetConfig if_name=%s\n", if_name);
  printf("config type=%d\n", config->type);
  printf("       wifi mode=%d\n", config->network.wifi.mode);
  printf("       wifi ssid=%s\n", config->network.wifi.ssid);
  printf("       wifi pass=%s\n", config->network.wifi.pass);
  return kPlErrCodeOk;
}

PlErrCode PlNetworkGetStatusStub(const char *if_name, PlNetworkStatus *status) {
  status->is_if_up = true;
  status->is_link_up = true;
  status->network.wifi.rssi = -12;
  status->network.wifi.band_width = kPlWifiBandWidthHt20;
  status->network.wifi.country.cc[0] = 'j';
  status->network.wifi.country.cc[1] = 'p';
  status->network.wifi.country.cc[2] = '\0';
  status->network.wifi.country.max_tx_power = 23;
  status->network.wifi.country.schan = 2;
  status->network.wifi.country.nchan = 5;
  status->network.wifi.country.policy = kPlWifiCountryPolicyAuto;
  return kPlErrCodeOk;
}
PlErrCode PlNetworkGetNetStatStub(char *buf, const uint32_t buf_size) {
  const char netstat[] =
      "             IPv4   TCP   UDP  ICMP\n"
      "Received     0026  001c  0009  0000\n"
      "Dropped      0001  0000  0000  0000\n"
      "  IPv4        VHL: 0000   Frg: 0000\n"
      "\n"
      "  Checksum   0000  0000  0000  ----\n"
      "  TCP         ACK: 0000   SYN: 0000\n"
      "              RST: 0002  0002\n"
      "  Type       0000  ----  ----  0000\n"
      "\n"
      "Sent         002c  0023  0009  0000\n"
      "  Rexmit     ----  0001  ----  ----\n";
  strncpy(buf, netstat, buf_size);
  return kPlErrCodeOk;
}

PlErrCode PlNetworkRegisterEventHandlerStub(const char *if_name,
                                            PlNetworkEventHandler handler,
                                            void *private_data) {
  if (handler_save == NULL) {
    printf("handler_save is NULL.\n");
    return kPlErrCodeError;
  }
  int i = 0;
  for (; i < kPlNetworkIfTypeStubMax; ++i) {
    if (strncmp(if_name, handler_save[i].if_name,
                sizeof(handler_save[i].if_name)) == 0) {
      printf("Handler register OK. if=%s handler=%p private_data=%p\n", if_name,
             handler, private_data);
      handler_save[i].handler = handler;
      handler_save[i].private_data = private_data;
    }
  }
  return kPlErrCodeOk;
}
PlErrCode PlNetworkUnregisterEventHandlerStub(const char *if_name) {
  if (handler_save == NULL) {
    printf("handler_save is NULL.\n");
    return kPlErrCodeError;
  }
  int i = 0;
  for (; i < kPlNetworkIfTypeStubMax; ++i) {
    if (strncmp(if_name, handler_save[i].if_name,
                sizeof(handler_save[i].if_name)) == 0) {
      printf("Handler Unregister OK. if=%s handler=%p private_data=%p\n",
             if_name, handler_save[i].handler, handler_save[i].private_data);
      handler_save[i].handler = NULL;
      handler_save[i].private_data = NULL;
    }
  }
  return kPlErrCodeOk;
}
PlErrCode PlNetworkStartStub(const char *if_name) {
  printf("PlNetworkStart if_name=%s\n", if_name);
#if 0
  int ret = 0;
  if ((ret = netlib_ifup(if_name)) != 0) {
    printf("ERROR: netlib_ifup failed. if=%s ret=%d errno=%d", if_name, ret,
           errno);
    return kPlErrInternal;
  }
#endif
  return kPlErrCodeOk;
}
PlErrCode PlNetworkStopStub(const char *if_name) {
  printf("PlNetworkStop if_name=%s\n", if_name);
#if 0
  int ret = 0;
  if ((ret = netlib_ifdown(if_name)) != 0) {
    printf("ERROR: netlib_ifdown failed. if=%s ret=%d errno=%d", if_name, ret,
           errno);
    return kPlErrInternal;
  }
#endif
  return kPlErrCodeOk;
}
PlErrCode PlNetworkStructInitializeStub(void *structure,
                                        PlNetworkStructType type) {
  if (system_info == NULL) {
    printf("system_info is NULL\n");
    return kPlErrCodeError;
  }
  switch (type) {
    case kPlNetworkStructTypeSystemInfo:
      PlNetworkSystemInfo *local_system_info = (PlNetworkSystemInfo *)structure;
      memset(local_system_info->if_name, '\0',
             sizeof(local_system_info->if_name));
      local_system_info->type = kPlNetworkTypeMax;
      local_system_info->cloud_enable = false;
      local_system_info->local_enable = false;
      break;
    case kPlNetworkStructTypeWifiConfig:
      PlNetworkWifiConfig *wifi_config = (PlNetworkWifiConfig *)structure;
      wifi_config->mode = kPlNetworkWifiModeMax;
      memset(wifi_config->ssid, '\0', sizeof(wifi_config->ssid));
      memset(wifi_config->pass, '\0', sizeof(wifi_config->pass));
      break;
    case kPlNetworkStructTypeWifiStatus:
      PlNetworkWifiStatus *wifi_status = (PlNetworkWifiStatus *)structure;
      wifi_status->rssi = 0;
      wifi_status->band_width = kPlWifiBandWidthHt20;
      snprintf(wifi_status->country.cc, sizeof(wifi_status->country.cc), "01");
      wifi_status->country.schan = 1;
      wifi_status->country.nchan = 11;
      wifi_status->country.max_tx_power = 0;
      wifi_status->country.policy = kPlWifiCountryPolicyManual;
      break;
    default:
      return kPlErrInvalidParam;
  }
  return kPlErrCodeOk;
}

void PlNetworkInitStub(void) {
  if (handler_save == NULL) {
    handler_save = (PlNetworkStubEventHandlerSave *)malloc(
        sizeof(PlNetworkStubEventHandlerSave) * kPlNetworkIfTypeStubMax);
    if (handler_save == NULL) {
      printf("handler_save allocate error.\n");
      PlNetworkDeinitStub();
      return;
    }
  }
  if (system_info == NULL) {
    system_info = (PlNetworkSystemInfo *)malloc(sizeof(PlNetworkSystemInfo) *
                                                kPlNetworkIfTypeStubMax);
    if (system_info == NULL) {
      printf("system_info allocate error.\n");
      PlNetworkDeinitStub();
      return;
    }
  }

  int i = 0;
  for (; i < kPlNetworkIfTypeStubMax; ++i) {
    switch (i) {
      case kPlNetworkIfTypeStubEther:
        snprintf(handler_save[i].if_name, sizeof(handler_save[i].if_name), "%s",
                 ETHER_IF_NAME);
        snprintf(system_info[i].if_name, sizeof(system_info[i].if_name), "%s",
                 ETHER_IF_NAME);
        system_info[i].type = kPlNetworkTypeEther;
        system_info[i].cloud_enable = true;
        system_info[i].local_enable = false;
        break;
      case kPlNetworkIfTypeStubWifiSta:
        snprintf(handler_save[i].if_name, sizeof(handler_save[i].if_name), "%s",
                 "wlan0");
        snprintf(system_info[i].if_name, sizeof(system_info[i].if_name), "%s",
                 "wlan0");
        system_info[i].type = kPlNetworkTypeWifi;
        system_info[i].cloud_enable = true;
        system_info[i].local_enable = false;
        break;
      case kPlNetworkIfTypeStubWifiAp:
        snprintf(handler_save[i].if_name, sizeof(handler_save[i].if_name), "%s",
                 "wlan1");
        snprintf(system_info[i].if_name, sizeof(system_info[i].if_name), "%s",
                 "wlan1");
        system_info[i].type = kPlNetworkTypeWifi;
        system_info[i].cloud_enable = false;
        system_info[i].local_enable = true;
        break;
      default:
        printf("Index invalid %d\n", i);
        PlNetworkDeinitStub();
        return;
    }
    handler_save[i].handler = NULL;
    handler_save[i].private_data = NULL;
  }
}
void PlNetworkDeinitStub(void) {
  if (handler_save) {
    free(handler_save);
    handler_save = NULL;
  }
  if (system_info) {
    free(system_info);
    system_info = NULL;
  }
}

void PlNetworkCallEventHandlerStub(PlNetworkIfTypeStub if_type,
                                   PlNetworkEvent event) {
  if (handler_save == NULL) {
    printf("handler_save is NULL.\n");
    return;
  }
  if (handler_save[if_type].handler != NULL) {
    printf("Call Event Handler. if=%s event=%d handler=%p private_data=%p\n",
           handler_save[if_type].if_name, event, handler_save[if_type].handler,
           handler_save[if_type].private_data);
    handler_save[if_type].handler(handler_save[if_type].if_name, event,
                                  handler_save[if_type].private_data);
  } else {
    printf("No Registered Handler. if=%s event=%d\n",
           handler_save[if_type].if_name, event);
  }
}

int dhcpc_request_stub(void *handle, struct dhcpc_state *p_result) {
  printf("dhcpc_request_stub. handle=%p p_result=%p\n", handle, p_result);
  if (!inet_aton("10.20.30.40", &p_result->ipaddr)) {
    printf("Ip address convert error.(src=%s)\n", "10.20.30.40");
    return 1;
  }
  if (!inet_aton("255.255.254.0", &p_result->netmask)) {
    printf("Ip address convert error.(src=%s)\n", "255.255.254.0");
    return 1;
  }
  if (!inet_aton("10.20.30.1", &p_result->default_router)) {
    printf("Ip address convert error.(src=%s)\n", "10.20.30.1");
    return 1;
  }
  if (!inet_aton("8.8.8.8", &p_result->dnsaddr)) {
    printf("Ip address convert error.(src=%s)\n", "8.8.8.8");
    return 1;
  }
  if (!inet_aton("10.20.30.254", &p_result->serverid)) {
    printf("Ip address convert error.(src=%s)\n", "10.20.30.254");
    return 1;
  }
  p_result->lease_time = 1234567890;
  return 0;
}

PlErrCode PlNetworkInitializeStub(void) {
  PlNetworkInitStub();
  return kPlErrCodeOk;
}

PlErrCode PlNetworkFinalizeStub(void) {
  PlNetworkDeinitStub();
  return kPlErrCodeOk;
}
