/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// self header.
#include "network_manager/network_manager_accessor.h"

// system header
#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// other header
#include "led_manager.h"
#include "porting_layer/include/pl.h"
#include "porting_layer/include/pl_network.h"
#include "network_manager/network_manager_internal.h"
#include "network_manager/network_manager_log.h"

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

#ifdef CONFIG_EXTERNAL_NETWORK_MANAGER_HAL_DUMMY
#define NETWORK_MANAGER_DHCPC_REQUEST dhcpc_request_stub
#define NETWORK_MANAGER_PL_NETWORK_GET_SYSTEM_INFO PlNetworkGetSystemInfoStub
#define NETWORK_MANAGER_PL_NETWORK_SET_CONFIG PlNetworkSetConfigStub
#define NETWORK_MANAGER_PL_NETWORK_GET_STATUS PlNetworkGetStatusStub
#define NETWORK_MANAGER_PL_NETWORK_GET_NET_STAT PlNetworkGetNetStatStub
#define NETWORK_MANAGER_PL_NETWORK_REGISTER_EVENT_HANDLER \
  PlNetworkRegisterEventHandlerStub
#define NETWORK_MANAGER_PL_NETWORK_UNREGISTER_EVENT_HANDLER \
  PlNetworkUnregisterEventHandlerStub
#define NETWORK_MANAGER_PL_NETWORK_START PlNetworkStartStub
#define NETWORK_MANAGER_PL_NETWORK_STOP PlNetworkStopStub
#define NETWORK_MANAGER_PL_NETWORK_INITIALIZE PlNetworkInitializeStub
#define NETWORK_MANAGER_PL_NETWORK_FINALIZE PlNetworkFinalizeStub
#else
#define NETWORK_MANAGER_PL_NETWORK_GET_SYSTEM_INFO PlNetworkGetSystemInfo
#define NETWORK_MANAGER_PL_NETWORK_SET_CONFIG PlNetworkSetConfig
#define NETWORK_MANAGER_PL_NETWORK_GET_STATUS PlNetworkGetStatus
#define NETWORK_MANAGER_PL_NETWORK_GET_NET_STAT PlNetworkGetNetStat
#define NETWORK_MANAGER_PL_NETWORK_REGISTER_EVENT_HANDLER \
  PlNetworkRegisterEventHandler
#define NETWORK_MANAGER_PL_NETWORK_UNREGISTER_EVENT_HANDLER \
  PlNetworkUnregisterEventHandler
#define NETWORK_MANAGER_PL_NETWORK_START PlNetworkStart
#define NETWORK_MANAGER_PL_NETWORK_STOP PlNetworkStop
#define NETWORK_MANAGER_PL_NETWORK_INITIALIZE PlNetworkInitialize
#define NETWORK_MANAGER_PL_NETWORK_FINALIZE PlNetworkFinalize
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_HAL_DUMMY

#define DHCP_RENEWAL_TIME_RATIO (0.5)      // T1: Renewal time
#define DHCP_REBINDING_TIME_RATIO (0.875)  // T2: Rebinding time

// Internal IP information structure.
typedef struct EsfNetworkManagerInternalIPInfo {
  struct in_addr ip;
  struct in_addr subnet_mask;
  struct in_addr gateway;
  struct in_addr dns;
} EsfNetworkManagerInternalIPInfo;

typedef struct {
  sem_t sem;
  bool done;
} WaitEventHandle;

// Invalid IP address definition.
static const EsfNetworkManagerIPInfo kEsfNetworkManagerInvalidIp = {
    {"0.0.0.0"}, {"0.0.0.0"}, {"0.0.0.0"}, {"0.0.0.0"}};

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
// AP address default definition
static const EsfNetworkManagerIPInfo kEsfNetworkManagerDefaultApIp = {
    {"192.168.4.1"}, {"255.255.255.0"}, {"192.168.4.1"}, {"0.0.0.0"}};
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

static WaitEventHandle s_if_down_pre_event_handle = {0};
static void    *s_dhcp_handle = NULL;
static uint64_t s_dhcp_lease_time = 0;
static uint64_t s_dhcp_t1_time = 0;
static uint64_t s_dhcp_t2_time = 0;
static uint64_t s_dhcp_update_time = 0;
static uint64_t s_t1_fail_time = 0;
static uint64_t s_t2_fail_time = 0;
static const uint64_t kDhcpcRenewRetrySec = 60;
struct PlNetworkDhcpcState s_dhcp_status = {0};

static void EsfNetworkManagerShowPlSystemInfo(uint32_t info_total_num,
                                              const PlNetworkSystemInfo *infos);
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpConvertIpAddress(
    const EsfNetworkManagerIPInfo *string_ip,
    EsfNetworkManagerInternalIPInfo *converted_ip);
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpConvertToString(
    const EsfNetworkManagerInternalIPInfo *src_ip,
    EsfNetworkManagerIPInfo *converted_string_ip);
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpSetIpAddress(
    const char *if_name, const EsfNetworkManagerInternalIPInfo *set_ip_address,
    bool is_ap);
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpSetDhcpAddress(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static EsfNetworkManagerResult EsfNetworkManagerAccessorStartHal(
    const char *if_name, const PlNetworkConfig *pl_config,
    const EsfNetworkManagerModeInfo *mode_info,
    PlNetworkEventHandler pl_event_handler);
static EsfNetworkManagerResult EsfNetworkManagerAccessorNormalStart(
    const EsfNetworkManagerOSInfo *connect_info,
    EsfNetworkManagerHandleInternal *handle_internal);
static EsfNetworkManagerResult EsfNetworkManagerAccessorWifiApStart(
    const EsfNetworkManagerOSInfo *connect_info,
    EsfNetworkManagerHandleInternal *handle_internal);
static void EsfNetworkManagerAccessorEtherIfLinkupEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorEtherIfDownEventOnDisconnecting(
    EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorEtherLinkdownEventOnConnected(
    EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorLeaseEvent(
    const char *if_name, EsfNetworkManagerModeInfo *mode_info);
static void EsfNetworkManagerAccessorEtherEventHandler(const char *if_name,
                                                       PlNetworkEvent event,
                                                       void *private_data);

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiStationConnectedEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiStationDisconnectedEventOnConnected(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiStationDisconnectedEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiStationStopEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiStationEventHandler(
    const char *if_name, PlNetworkEvent event, void *private_data);
#else
#define EsfNetworkManagerAccessorWifiStationEventHandler (NULL)
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpSetDhcpdAddress(
    const EsfNetworkManagerInternalIPInfo *set_ip_address);
static void EsfNetworkManagerAccessorWifiApStartEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiApConnectedEventOnConnecting(
    EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiApConnectedEventOnConnected(
    EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiApDisconnectedEventOnConnected(
    EsfNetworkManagerModeInfo *mode);
static void EsfNetworkManagerAccessorWifiApEventHandler(const char *if_name,
                                                        PlNetworkEvent event,
                                                        void *private_data);
#else
#define EsfNetworkManagerAccessorWifiApEventHandler (NULL)
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

// """Show PL system information.

// Show PL system information.

// Args:
//     info_total_num (int32_t): Specify the number of PL system information.
//     infos (const PlNetworkSystemInfo *):
//       Specifies the PL system information array.

// Note:
// """
static void EsfNetworkManagerShowPlSystemInfo(
    uint32_t info_total_num, const PlNetworkSystemInfo *infos) {
  ESF_NETWORK_MANAGER_TRACE("START");
  ESF_NETWORK_MANAGER_DBG("  info_num=%u", info_total_num);
  uint32_t i = 0;
  for (; i < info_total_num; ++i) {
    ESF_NETWORK_MANAGER_DBG("    [%u] if_name=%s", i, infos[i].if_name);
    ESF_NETWORK_MANAGER_DBG("    [%u] type=%d", i, infos[i].type);
    ESF_NETWORK_MANAGER_DBG("    [%u] cloud_enable=%d", i,
                            infos[i].cloud_enable);
    ESF_NETWORK_MANAGER_DBG("    [%u] local_enable=%d", i,
                            infos[i].local_enable);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

// """Convert IP address from string to binary.

// Convert IP address from string to binary.
// IP address is set in network endianness.

// Args:
//     string_ip (const EsfNetworkManagerIPInfo *): Source IP address.
//     converted_ip (EsfNetworkManagerInternalIPInfo *): Destination IP address.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpConvertIpAddress(
    const EsfNetworkManagerIPInfo *string_ip,
    EsfNetworkManagerInternalIPInfo *converted_ip) {
  ESF_NETWORK_MANAGER_TRACE("START string_ip=%p converted_ip=%p", string_ip,
                            converted_ip);
  if (string_ip == NULL || converted_ip == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(string_ip=%p converted_ip=%p)",
                            string_ip, converted_ip);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_DBG("convert ip ip=%s mask=%s gateway=%s dns=%s",
                          string_ip->ip, string_ip->subnet_mask,
                          string_ip->gateway, string_ip->dns);
  if (!inet_aton(string_ip->ip, &converted_ip->ip)) {
    ESF_NETWORK_MANAGER_ERR("Ip address convert error.(src=%s)", string_ip->ip);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  if (!inet_aton(string_ip->subnet_mask, &converted_ip->subnet_mask)) {
    ESF_NETWORK_MANAGER_ERR("Ip address mask convert error.(src=%s)",
                            string_ip->subnet_mask);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  if (!inet_aton(string_ip->gateway, &converted_ip->gateway)) {
    ESF_NETWORK_MANAGER_ERR("Ip address gateway convert error.(src=%s)",
                            string_ip->gateway);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  if (!inet_aton(string_ip->dns, &converted_ip->dns)) {
    ESF_NETWORK_MANAGER_ERR("Ip address dns convert error.(src=%s)",
                            string_ip->dns);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_DBG(
      "converted ip ip=0x%x mask=0x%x gateway=0x%x dns=0x%x",
      converted_ip->ip.s_addr, converted_ip->subnet_mask.s_addr,
      converted_ip->gateway.s_addr, converted_ip->dns.s_addr);
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

// """Convert IP address from binary to string.

// Convert IP address from binary to string.

// Args:
//     src_ip (const EsfNetworkManagerInternalIPInfo *): Source IP address.
//     converted_string_ip (EsfNetworkManagerIPInfo *): Destination IP address.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpConvertToString(
    const EsfNetworkManagerInternalIPInfo *src_ip,
    EsfNetworkManagerIPInfo *converted_string_ip) {
  ESF_NETWORK_MANAGER_TRACE("START src_ip=%p converted_string_ip=%p", src_ip,
                            converted_string_ip);
  if (src_ip == NULL || converted_string_ip == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(src_ip=%p converted_ip=%p)",
                            src_ip, converted_string_ip);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_DBG("convert ip ip=0x%x mask=0x%x gateway=0x%x dns=0x%x",
                          src_ip->ip.s_addr, src_ip->subnet_mask.s_addr,
                          src_ip->gateway.s_addr, src_ip->dns.s_addr);
  const char *ret_ntop = inet_ntop(AF_INET, &src_ip->ip,
                                   converted_string_ip->ip,
                                   sizeof(converted_string_ip->ip));
  if (ret_ntop == NULL) {
    ESF_NETWORK_MANAGER_ERR("Ip address convert error. 0x%x",
                            src_ip->ip.s_addr);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ret_ntop = inet_ntop(AF_INET, &src_ip->subnet_mask,
                       converted_string_ip->subnet_mask,
                       sizeof(converted_string_ip->subnet_mask));
  if (ret_ntop == NULL) {
    ESF_NETWORK_MANAGER_ERR("Ip address mask convert error. 0x%x",
                            src_ip->subnet_mask.s_addr);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ret_ntop = inet_ntop(AF_INET, &src_ip->gateway, converted_string_ip->gateway,
                       sizeof(converted_string_ip->gateway));
  if (ret_ntop == NULL) {
    ESF_NETWORK_MANAGER_ERR("Ip address gateway convert error. 0x%x",
                            src_ip->gateway.s_addr);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ret_ntop = inet_ntop(AF_INET, &src_ip->dns, converted_string_ip->dns,
                       sizeof(converted_string_ip->dns));
  if (ret_ntop == NULL) {
    ESF_NETWORK_MANAGER_ERR("Ip address dns convert error. 0x%x",
                            src_ip->dns.s_addr);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_DBG(
      "converted ip ip=%s mask=%s gateway=%s dns=%s", converted_string_ip->ip,
      converted_string_ip->subnet_mask, converted_string_ip->gateway,
      converted_string_ip->dns);
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

// """Set the IP address for the specified interface.

// Set the IP address, netmask, default gateway, and DNS.
// When AP is specified, DNS is not set.

// Args:
//     if_name (const char *): Setting destination interface.
//     set_ip_address (const EsfNetworkManagerInternalIPInfo *):
//       Setting IP address information.
//     is_ap (bool): AP mode identification flag.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultUtilityIPAddressError: IP address setting library
//     error. kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpSetIpAddress(
    const char *if_name, const EsfNetworkManagerInternalIPInfo *set_ip_address,
    bool is_ap) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p set_ip_address=%p", if_name,
                            set_ip_address);
  if (if_name == NULL || set_ip_address == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p set_ip_address=%p)",
                            if_name, set_ip_address);
    return kEsfNetworkManagerResultInternalError;
  }
  ESF_NETWORK_MANAGER_INFO(
      "if_name=%s ip_address=0x%x ipv4netmask=0x%x gateway=0x%x dns=0x%x",
      if_name, set_ip_address->ip.s_addr, set_ip_address->subnet_mask.s_addr,
      set_ip_address->gateway.s_addr, set_ip_address->dns.s_addr);
  int ret = 0;
  ESF_NETWORK_MANAGER_TRACE("set ipv4 address if_name=%s ip_address=0x%x",
                            if_name, set_ip_address->ip.s_addr);
  ESF_NETWORK_MANAGER_TRACE("set ipv4 address if_name=%s ipv4netmask=0x%x",
                            if_name, set_ip_address->subnet_mask.s_addr);
  ret = PlNetworkSetIpv4Addr(if_name, &set_ip_address->ip,
                             &set_ip_address->subnet_mask);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("netlib_set_ipv4addr failed. ret=%d errno=%d", ret,
                            errno);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_IP_FAILURE);
    return kEsfNetworkManagerResultUtilityIPAddressError;
  }
  ESF_NETWORK_MANAGER_TRACE("set ipv4 address if_name=%s gateway=0x%x", if_name,
                            set_ip_address->gateway.s_addr);
  ret = PlNetworkSetIpv4Gateway(if_name, &set_ip_address->gateway);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("netlib_set_dripv4addr failed. ret=%d errno=%d",
                            ret, errno);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_IP_FAILURE);
    return kEsfNetworkManagerResultUtilityIPAddressError;
  }
  if (!is_ap) {
    // Reset DNS settings to default.
    // Ignore the return value as an error may occur depending on CONFIG.
    ret = PlNetworkResetDnsServer();
    ESF_NETWORK_MANAGER_TRACE("dns_default_nameserver ret=%d", ret);
    ESF_NETWORK_MANAGER_TRACE("set ipv4 address if_name=%s dns=0x%x", if_name,
                              set_ip_address->dns.s_addr);
    ret = PlNetworkSetIpv4Dns(if_name, &set_ip_address->dns);
    if (ret != 0) {
      ESF_NETWORK_MANAGER_ERR("netlib_set_ipv4dnsaddr failed. ret=%d errno=%d",
                              ret, errno);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_IP_FAILURE);
      return kEsfNetworkManagerResultUtilityIPAddressError;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

// """Set the specified IP address to dhcpd.

// Set the IP address, netmask, default gateway, and DNS.
// For the IP address, use the specified value +1.

// Args:
//     set_ip_address (const EsfNetworkManagerInternalIPInfo *):
//       Setting IP address information.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultUtilityDHCPServerError: dhcpd error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpSetDhcpdAddress(
    const EsfNetworkManagerInternalIPInfo *set_ip_address) {
  ESF_NETWORK_MANAGER_TRACE("START set_ip_address=%p", set_ip_address);
  (void)set_ip_address;
  if (set_ip_address == NULL) {
    ESF_NETWORK_MANAGER_WARN("parameter error.(set_ip_address=%p)",
                             set_ip_address);
    return kEsfNetworkManagerResultInternalError;
  }

  int ret = 0;
  struct in_addr address = set_ip_address->ip;
  // dhcpd settings are host endian so convert them.
  // Use IF setting +1 for the address issued by dhcpd.
  address.s_addr = ntohl(address.s_addr) + 1u;
  if ((ret = dhcpd_set_startip(address.s_addr)) != 0) {
    ESF_NETWORK_MANAGER_WARN("dhcpd_set_startip failed. ret=%d errno=%d", ret,
                             errno);
    return kEsfNetworkManagerResultUtilityDHCPServerError;
  }
  address = set_ip_address->subnet_mask;
  address.s_addr = ntohl(address.s_addr);
  if ((ret = dhcpd_set_netmask(address.s_addr)) != 0) {
    ESF_NETWORK_MANAGER_WARN("dhcpd_set_netmask failed. ret=%d errno=%d", ret,
                             errno);
    return kEsfNetworkManagerResultUtilityDHCPServerError;
  }
  address = set_ip_address->gateway;
  address.s_addr = ntohl(address.s_addr);
  if ((ret = dhcpd_set_routerip(address.s_addr)) != 0) {
    ESF_NETWORK_MANAGER_WARN("dhcpd_set_routerip failed. ret=%d errno=%d", ret,
                             errno);
    return kEsfNetworkManagerResultUtilityDHCPServerError;
  }
  address = set_ip_address->dns;
  address.s_addr = ntohl(address.s_addr);
  if ((ret = dhcpd_set_dnsip(address.s_addr)) != 0) {
    ESF_NETWORK_MANAGER_WARN("dhcpd_set_dnsip failed. ret=%d errno=%d", ret,
                             errno);
    return kEsfNetworkManagerResultUtilityDHCPServerError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

static bool IsValidDhcpTime(void) {
  // Valid check of server value
  // expect: (t1 < t2 < lease)
  return (s_dhcp_status.renewal_time < s_dhcp_status.rebinding_time) &&
         (s_dhcp_status.rebinding_time < s_dhcp_status.lease_time);
}

static bool ShouldRenewDhcp(uint64_t now_sec, uint64_t previous_sec) {
  if (previous_sec == 0) {
    return true;
  }
  uint64_t elapsed = now_sec - previous_sec;
  return elapsed > kDhcpcRenewRetrySec;
}

static void WaitEventHandleInit(WaitEventHandle *h) {
  sem_init(&h->sem, 0, 0);
  h->done = false;
  return;
}

static void WaitEvent(WaitEventHandle *h) {
  while (!h->done) {
    sem_wait(&h->sem);
  }
  h->done = false;
  return;
}

static void CompleteEvent(WaitEventHandle *h) {
  h->done = true;
  sem_post(&h->sem);
  return;
}

static void WaitEventHandleDeInit(WaitEventHandle *h) {
  sem_destroy(&h->sem);
  return;
}

static EsfNetworkManagerResult RequestDhcp(const char *if_name,
                                           EsfNetworkManagerModeInfo *mode) {
  s_dhcp_status.lease_time = 0;
  s_dhcp_status.renewal_time = 0;
  s_dhcp_status.rebinding_time = 0;

  int plret = PlNetworkDhcpcRequest(s_dhcp_handle, &s_dhcp_status);
  if (plret != 0) {
    ESF_NETWORK_MANAGER_ERR("dhcpc_request error.(if=%s, ret=%d, errno=%d)",
                            if_name, plret, errno);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_DHCP_FAILURE);
    return kEsfNetworkManagerResultUtilityIPAddressError;
  }
  s_dhcp_lease_time = s_dhcp_status.lease_time;
  s_dhcp_t1_time = s_dhcp_status.renewal_time;
  s_dhcp_t2_time = s_dhcp_status.rebinding_time;
  if (!IsValidDhcpTime()) {
    s_dhcp_t1_time = s_dhcp_lease_time * DHCP_RENEWAL_TIME_RATIO;
    s_dhcp_t2_time = s_dhcp_lease_time * DHCP_REBINDING_TIME_RATIO;
  }
  ESF_NETWORK_MANAGER_INFO("Lease:%" PRIu64 "[s] T1:%" PRIu64 "[s] T2:%" PRIu64
                           "[s]\n",
                           s_dhcp_lease_time, s_dhcp_t1_time, s_dhcp_t2_time);
  EsfNetworkManagerInternalIPInfo ip_info = {0};
  ip_info.ip = s_dhcp_status.ipaddr;
  ip_info.subnet_mask = s_dhcp_status.netmask;
  ip_info.gateway = s_dhcp_status.default_router;
  ip_info.dns = s_dhcp_status.dnsaddr;
  EsfNetworkManagerResult ret;
  ret = EsfNetworkManagerAccessorIpSetIpAddress(if_name, &ip_info, false);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("IP address set error.(if=%s, ret=%u)", if_name,
                            ret);
  }
  if (ret == kEsfNetworkManagerResultSuccess) {
    ret = EsfNetworkManagerAccessorIpConvertToString(&ip_info, &mode->ip_info);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("IP address save local error.(if=%s)", if_name);
    }
  }
  struct timespec abstime;
  clock_gettime(CLOCK_MONOTONIC, &abstime);
  s_dhcp_update_time = abstime.tv_sec;

  ESF_NETWORK_MANAGER_INFO(
      "DHCPC request done. Next update:%" PRIu64 "[s] later", s_dhcp_t1_time);
  return ret;
}

static void CloseDhcp(void) {
  if (s_dhcp_handle == NULL) {
    return;
  }
  PlNetworkDhcpcClose(s_dhcp_handle);
  s_dhcp_handle = NULL;
  s_dhcp_lease_time = 0;
  s_dhcp_update_time = 0;
  s_t1_fail_time = 0;
  s_t2_fail_time = 0;
  memset(&s_dhcp_status, 0, sizeof(s_dhcp_status));
  return;
}

// """Obtain an IP address via DHCP and set it to the specified interface.

// Obtain the IP address to use via DHCP.
// Set the obtained information to the specified interface.
// Save the obtained information to internal resources.

// Args:
//     if_name (const char *): Setting destination interface.
//     mode (EsfNetworkManagerModeInfo *): Destination resource information.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultUtilityIPAddressError: IP address library error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorIpSetDhcpAddress(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_WARN("parameter error.(if_name=%p mode=%p)", if_name,
                             mode);
    return kEsfNetworkManagerResultInternalError;
  }
  // Pass through snprintf to ensure NULL termination.
  char if_name_wk[PL_NETWORK_IFNAME_LEN + 1] = {
      '\0',
  };
  snprintf(if_name_wk, sizeof(if_name_wk), "%s", if_name);
  if_name_wk[PL_NETWORK_IFNAME_LEN] = '\0';
  EsfNetworkManagerResult ret_network = kEsfNetworkManagerResultSuccess;
  do {
    PlNetworkCapabilities capabilities = PlNetworkGetCapabilities();
    if (capabilities.use_external_dhcpc) {
      break;
    }
    struct in_addr ipv4_addr = {
        .s_addr = 0,
    };
    struct in_addr ipv4_netmask = {
        .s_addr = 0,
    };
    int ret = PlNetworkGetIpv4Addr(if_name_wk, &ipv4_addr, &ipv4_netmask);
    if (ret != 0) {
      ESF_NETWORK_MANAGER_ERR("netlib_get_ipv4addr error.(ret=%d)", ret);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_IP_FAILURE);
      ret_network = kEsfNetworkManagerResultUtilityIPAddressError;
      break;
    }
    if (ipv4_addr.s_addr == 0) {
      // dhcp request ip
      uint8_t mac[PL_NETWORK_IFHWADDRLEN] = {
          0,
      };
      ret = PlNetworkGetMacAddr(if_name_wk, mac);
      if (ret != 0) {
        ESF_NETWORK_MANAGER_ERR("netlib_getmacaddr error.(if=%s, ret=%d)",
                                if_name, ret);
        ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_IP_FAILURE);
        ret_network = kEsfNetworkManagerResultUtilityIPAddressError;
        break;
      }
      if (s_dhcp_handle == NULL) {
        s_dhcp_handle = PlNetworkDhcpcOpen(if_name_wk, &mac,
                                           PL_NETWORK_IFHWADDRLEN);
        if (s_dhcp_handle == NULL) {
          ESF_NETWORK_MANAGER_ERR("dhcpc_open failed.(if=%s)", if_name);
          ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_DHCP_FAILURE);
          ret_network = kEsfNetworkManagerResultUtilityIPAddressError;
          break;
        }
      }
      ret_network = RequestDhcp(if_name_wk, mode);
    }
  } while (0);

  ESF_NETWORK_MANAGER_TRACE("END");
  return ret_network;
}

// """Start the HAL network.

// This sets the configuration for the specified interface,
// registers event handlers, and opens a connection.

// Args:
//     if_name (const char *): Target interface.
//     pl_config (const PlNetworkConfig *):
//       Configuration information to be set.
//     mode_info (const EsfNetworkManagerModeInfo *):
//       Network resource information.
//     pl_event_handler(PlNetworkEventHandler):
//       Event handler to be registered.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: HAL api error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorStartHal(
    const char *if_name, const PlNetworkConfig *pl_config,
    const EsfNetworkManagerModeInfo *mode_info,
    PlNetworkEventHandler pl_event_handler) {
  ESF_NETWORK_MANAGER_TRACE(
      "START if_name=%p pl_config=%p mode_info=%p pl_event_handler=%p", if_name,
      pl_config, mode_info, pl_event_handler);
  if (if_name == NULL || pl_config == NULL || mode_info == NULL ||
      pl_event_handler == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(if_name=%p pl_config=%p mode_info=%p "
        "pl_event_handler=%p)",
        if_name, pl_config, mode_info, pl_event_handler);
    return kEsfNetworkManagerResultInternalError;
  }

  PlErrCode hal_ret = kPlErrCodeOk;
  do {
    hal_ret = NETWORK_MANAGER_PL_NETWORK_SET_CONFIG(if_name, pl_config);
    if (hal_ret != kPlErrCodeOk && hal_ret != kPlErrNoSupported) {
      ESF_NETWORK_MANAGER_ERR("HAL(PlNetworkSetConfig) error.(if=%s ret=%u)",
                              if_name, hal_ret);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
      break;
    }
    hal_ret = NETWORK_MANAGER_PL_NETWORK_UNREGISTER_EVENT_HANDLER(if_name);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG(
          "HAL(PlNetworkUnregisterEventHandler) error.(if=%s ret=%d)", if_name,
          hal_ret);
      // Ignore errors.
    }
    hal_ret = NETWORK_MANAGER_PL_NETWORK_REGISTER_EVENT_HANDLER(
        if_name, pl_event_handler, (void *)mode_info);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_ERR(
          "HAL(PlNetworkRegisterEventHandler) error.(if=%s ret=%u)", if_name,
          hal_ret);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
      break;
    }
    hal_ret = NETWORK_MANAGER_PL_NETWORK_START(if_name);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_ERR("HAL(PlNetworkStart) error.(if=%s ret=%u)",
                              if_name, hal_ret);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
      break;
    }
  } while (0);
  if (hal_ret != kPlErrCodeOk) {
    return kEsfNetworkManagerResultHWIFError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

// """Start a Normal mode connection.

// Start a Normal mode connection.

// Args:
//     connect_info (const EsfNetworkManagerOSInfo *): Connection information.
//     handle_internal (EsfNetworkManagerHandleInternal *): Internal handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInvalidParameter: Parameter error.
//     kEsfNetworkManagerResultUtilityIPAddressError: IP address setting library
//     error. kEsfNetworkManagerResultHWIFError: HAL api error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorNormalStart(
    const EsfNetworkManagerOSInfo *connect_info,
    EsfNetworkManagerHandleInternal *handle_internal) {
  ESF_NETWORK_MANAGER_TRACE("START connect_info=%p handle_internal=%p",
                            connect_info, handle_internal);
  if (connect_info == NULL || handle_internal == NULL ||
      handle_internal->mode_info == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(connect_info=%p handle_internal=%p)", connect_info,
        handle_internal);
    return kEsfNetworkManagerResultInternalError;
  }
  // clang-format off
  PlNetworkConfig pl_config = {
      .type = kPlNetworkTypeEther,
      .network = {.wifi = {.mode = kPlNetworkWifiModeSta, }, },
  };
  // clang-format on
  PlNetworkEventHandler pl_event_handler =
      EsfNetworkManagerAccessorEtherEventHandler;
  EsfNetworkManagerInterfaceKind interface_kind =
      kEsfNetworkManagerInterfaceKindEther;
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
  if (connect_info->normal_mode.netif_kind == kEsfNetworkManagerNetifKindWiFi) {
    pl_config.type = kPlNetworkTypeWifi;
    interface_kind = kEsfNetworkManagerInterfaceKindWifi;
    memcpy(pl_config.network.wifi.ssid, connect_info->normal_mode.wifi_sta.ssid,
           sizeof(pl_config.network.wifi.ssid));
    memcpy(pl_config.network.wifi.pass,
           connect_info->normal_mode.wifi_sta.password,
           sizeof(pl_config.network.wifi.pass));
    pl_event_handler = EsfNetworkManagerAccessorWifiStationEventHandler;
  }
#endif
  if (handle_internal->mode_info->hal_system_info[interface_kind] == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(netif_kind=%d)",
                            connect_info->normal_mode.netif_kind);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM);
    return kEsfNetworkManagerResultInvalidParameter;
  }

  const char *if_name =
      handle_internal->mode_info->hal_system_info[interface_kind]->if_name;
  const EsfNetworkManagerIPInfo *set_ip_info = NULL;
  switch (connect_info->normal_mode.ip_method) {
    case kEsfNetworkManagerIpMethodDhcp:
      set_ip_info = &kEsfNetworkManagerInvalidIp;
      break;
    case kEsfNetworkManagerIpMethodStatic:
      set_ip_info = &connect_info->normal_mode.dev_ip;
      break;
    default:
      ESF_NETWORK_MANAGER_ERR("parameter error.(ip_method=%d)",
                              connect_info->normal_mode.ip_method);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM);
      return kEsfNetworkManagerResultInvalidParameter;
  }
  // clang-format off
  EsfNetworkManagerInternalIPInfo converted_ip = {
      .ip = {.s_addr = 0, },
  };
  // clang-format on
  EsfNetworkManagerResult ret =
      EsfNetworkManagerAccessorIpConvertIpAddress(set_ip_info, &converted_ip);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Convert ip address error.");
    return ret;
  }

  bool is_dhcp =
      (connect_info->normal_mode.ip_method == kEsfNetworkManagerIpMethodDhcp);
  // The order in which the functions are called is changed
  // depending on whether it is DHCP or not.
  // This is because an error occurs when libnm checks the relationship
  // between ip_method and the IP address.
  if (is_dhcp) {
    // You cannot clear the IP address when ip_method = "manual"
    int r = PlNetworkSetIpv4Method(if_name, is_dhcp);
    if (r != 0) {
      ESF_NETWORK_MANAGER_ERR("Set ip method error.");
      return kEsfNetworkManagerResultInternalError;
    }
    ret = EsfNetworkManagerAccessorIpSetIpAddress(if_name, &converted_ip,
                                                  false);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Set ip address error.");
      return ret;
    }
  } else {
    // You cannot set ip_method = "manual" until you have set the IP address.
    ret = EsfNetworkManagerAccessorIpSetIpAddress(if_name, &converted_ip,
                                                  false);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Set ip address error.");
      return ret;
    }
    int r = PlNetworkSetIpv4Method(if_name, is_dhcp);
    if (r != 0) {
      ESF_NETWORK_MANAGER_ERR("Set ip method error.");
      return kEsfNetworkManagerResultInternalError;
    }
  }
  ret = EsfNetworkManagerAccessorStartHal(
      if_name, &pl_config, handle_internal->mode_info, pl_event_handler);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Hal control error.");
    return ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

// """Start a AccessPoint mode connection.

// Start a AccessPoint mode connection.

// Args:
//     connect_info (const EsfNetworkManagerOSInfo *): Connection information.
//     handle_internal (EsfNetworkManagerHandleInternal *): Internal handle.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultInvalidParameter: Parameter error.
//     kEsfNetworkManagerResultUtilityDHCPServerError: dhcpd error.
//     kEsfNetworkManagerResultUtilityIPAddressError: IP address setting library
//     error. kEsfNetworkManagerResultHWIFError: HAL api error.
//     kEsfNetworkManagerResultInternalError: Internal error.

// Note:
// """
static EsfNetworkManagerResult EsfNetworkManagerAccessorWifiApStart(
    const EsfNetworkManagerOSInfo *connect_info,
    EsfNetworkManagerHandleInternal *handle_internal) {
  ESF_NETWORK_MANAGER_TRACE("START connect_info=%p handle_internal=%p",
                            connect_info, handle_internal);
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
  if (connect_info == NULL || handle_internal == NULL ||
      handle_internal->mode_info == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(connect_info=%p handle_internal=%p)", connect_info,
        handle_internal);
    return kEsfNetworkManagerResultInternalError;
  }
  // clang-format off
  PlNetworkConfig pl_config = {
      .type = kPlNetworkTypeWifi,
      .network = {.wifi = {.mode = kPlNetworkWifiModeAp, }, },
  };
  // clang-format on
  EsfNetworkManagerInterfaceKind interface_kind =
      kEsfNetworkManagerInterfaceKindWifi;
  PlNetworkEventHandler pl_event_handler =
      EsfNetworkManagerAccessorWifiApEventHandler;
  memcpy(pl_config.network.wifi.ssid,
         connect_info->accesspoint_mode.wifi_ap.ssid,
         sizeof(pl_config.network.wifi.ssid));
  memcpy(pl_config.network.wifi.pass,
         connect_info->accesspoint_mode.wifi_ap.password,
         sizeof(pl_config.network.wifi.pass));
  if (handle_internal->mode_info->hal_system_info[interface_kind] == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(netif_kind=%d)",
                            connect_info->normal_mode.netif_kind);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM);
    return kEsfNetworkManagerResultInvalidParameter;
  }

  const char *if_name =
      handle_internal->mode_info->hal_system_info[interface_kind]->if_name;
  const EsfNetworkManagerIPInfo *set_ip_info = NULL;
  if (connect_info->accesspoint_mode.dev_ip.ip[0] == '\0') {
    set_ip_info = &kEsfNetworkManagerDefaultApIp;
  } else {
    set_ip_info = &connect_info->accesspoint_mode.dev_ip;
  }
  // clang-format off
  EsfNetworkManagerInternalIPInfo converted_ip = {.ip = {.s_addr = 0, }, };
  // clang-format on
  EsfNetworkManagerResult ret =
      EsfNetworkManagerAccessorIpConvertIpAddress(set_ip_info, &converted_ip);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Convert ip address error.");
    return ret;
  }

  ret = EsfNetworkManagerAccessorIpSetDhcpdAddress(&converted_ip);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Set dhcpd ip address error.");
    return ret;
  }

  ret = EsfNetworkManagerAccessorIpSetIpAddress(if_name, &converted_ip, true);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Set ip address error.");
    return ret;
  }

  ret = EsfNetworkManagerAccessorStartHal(
      if_name, &pl_config, handle_internal->mode_info, pl_event_handler);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Hal control error.");
    return ret;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
  return kEsfNetworkManagerResultSuccess;
}

// """IFup/Linkup event reception processing during connection attempt.

// When both IF/Link are in UP state, do the following.
// Update led manager.
// If using DHCP, obtain and set address.
// Invoke callback function.

// Args:
//     if_name (const char *): Target interface.
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
static void EsfNetworkManagerAccessorEtherIfLinkupEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p mode=%p)", if_name,
                            mode);
    return;
  }
  if (mode->status_info.is_if_up && mode->status_info.is_link_up) {
    if (mode->connect_info.normal_mode.ip_method ==
        kEsfNetworkManagerIpMethodDhcp) {
      EsfNetworkManagerAccessorSetLedManagerStatusService(
          kEsfLedManagerLedStatusDisconnectedNoInternetConnection, true);
      EsfNetworkManagerResult ret =
          EsfNetworkManagerAccessorIpSetDhcpAddress(if_name, mode);
      if (ret != kEsfNetworkManagerResultSuccess) {
        ESF_NETWORK_MANAGER_ERR("Dhcp ip address get error.");
        return;
      }
    }
    mode->connect_status = kEsfNetworkManagerConnectStatusConnected;
    mode->notify_info = kEsfNetworkManagerNotifyInfoConnected;
    ESF_NETWORK_MANAGER_INFO("Ether %s is connected.", if_name);
    ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                            mode->connect_status, mode->notify_info);
    EsfNetworkManagerAccessorSetLedManagerStatusService(
        kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, true);
    EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeNormal, mode);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

// """IFdown event reception processing during disconnection attempt.

// Update led manager.
// Invoke callback function.

// Args:
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
static void EsfNetworkManagerAccessorEtherIfDownEventOnDisconnecting(
    EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%p", mode);
  if (mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mode=%p)", mode);
    return;
  }
  CloseDhcp();
  mode->connect_status = kEsfNetworkManagerConnectStatusDisconnected;
  mode->notify_info = kEsfNetworkManagerNotifyInfoDisconnected;
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          mode->connect_status, mode->notify_info);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedNoInternetConnection, false);
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeNormal, mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

// """Linkdown event reception processing during connected.

// Update led manager.
// Invoke callback function.

// Args:
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
static void EsfNetworkManagerAccessorEtherLinkdownEventOnConnected(
    EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%p", mode);
  if (mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mode=%p)", mode);
    return;
  }
  CloseDhcp();
  mode->connect_status = kEsfNetworkManagerConnectStatusConnecting;
  mode->notify_info = kEsfNetworkManagerNotifyInfoDisconnected;
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          mode->connect_status, mode->notify_info);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedNoInternetConnection, false);
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeNormal, mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

// """Wifi ST connected event reception processing during connection attempt.

// Update led manager.
// If using DHCP, obtain and set address.
// Invoke callback function.

// Args:
//     if_name (const char *): Target interface.
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiStationConnectedEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p mode=%p)", if_name,
                            mode);
    return;
  }
  if (mode->connect_info.normal_mode.ip_method ==
      kEsfNetworkManagerIpMethodDhcp) {
    EsfNetworkManagerAccessorSetLedManagerStatusService(
        kEsfLedManagerLedStatusDisconnectedNoInternetConnection, true);
    EsfNetworkManagerResult ret =
        EsfNetworkManagerAccessorIpSetDhcpAddress(if_name, mode);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("Dhcp ip address get error. %s", if_name);
      EsfNetworkManagerAccessorSetLedManagerStatus(
          kEsfNetworkManagerModeNormal,
          kEsfNetworkManagerAccessorLedManagerStatusStartError,
          kEsfNetworkManagerNetifKindWiFi);
      return;
    }
  }
  mode->connect_status = kEsfNetworkManagerConnectStatusConnected;
  mode->notify_info = kEsfNetworkManagerNotifyInfoConnected;
  ESF_NETWORK_MANAGER_INFO("WiFi %s is connected.", if_name);
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          mode->connect_status, mode->notify_info);
  EsfNetworkManagerAccessorSetLedManagerStatus(
      kEsfNetworkManagerModeNormal,
      kEsfNetworkManagerAccessorLedManagerStatusConnected,
      kEsfNetworkManagerNetifKindWiFi);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, true);
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeNormal, mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE

// """WiFi ST disconnected event reception processing during connected.

// Disconnect from WiFi.
// Update led manager.
// Invoke callback function.

// Args:
//     if_name (const char *): Target interface.
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiStationDisconnectedEventOnConnected(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p mode=%p)", if_name,
                            mode);
    return;
  }

  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_STOP(if_name);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkStop failed. if=%s ret=%u", if_name,
                            hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    EsfNetworkManagerAccessorSetLedManagerStatus(
        kEsfNetworkManagerModeNormal,
        kEsfNetworkManagerAccessorLedManagerStatusStartError,
        kEsfNetworkManagerNetifKindWiFi);
    return;
  }

  mode->connect_status = kEsfNetworkManagerConnectStatusConnecting;
  mode->notify_info = kEsfNetworkManagerNotifyInfoDisconnected;
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          mode->connect_status, mode->notify_info);

  EsfNetworkManagerAccessorSetLedManagerStatus(
      kEsfNetworkManagerModeNormal,
      kEsfNetworkManagerAccessorLedManagerStatusConnecting,
      kEsfNetworkManagerNetifKindWiFi);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedNoInternetConnection, false);
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeNormal, mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE

// """WiFi ST disconnected event reception processing during connecting.

// Disconnect from WiFi.
// Update led manager.
// Invoke callback function.

// Args:
//     if_name (const char *): Target interface.
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiStationDisconnectedEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p mode=%p)", if_name,
                            mode);
    return;
  }

  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_STOP(if_name);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkStop failed. if=%s ret=%u", if_name,
                            hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    EsfNetworkManagerAccessorSetLedManagerStatus(
        kEsfNetworkManagerModeNormal,
        kEsfNetworkManagerAccessorLedManagerStatusStartError,
        kEsfNetworkManagerNetifKindWiFi);
    return;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE

// """WiFi ST stop event reception processing during connecting.

// Reconnect WiFi.

// Args:
//     if_name (const char *): Target interface.
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiStationStopEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p mode=%p)", if_name,
                            mode);
    return;
  }
  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_START(if_name);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkStart failed. if=%s ret=%u", if_name,
                            hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    EsfNetworkManagerAccessorSetLedManagerStatus(
        kEsfNetworkManagerModeNormal,
        kEsfNetworkManagerAccessorLedManagerStatusStartError,
        kEsfNetworkManagerNetifKindWiFi);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE

// """Wifi AP start event reception processing during connection attempt.

// Start dhcpd.
// Invoke callback function.

// Args:
//     if_name (const char *): Target interface.
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiApStartEventOnConnecting(
    const char *if_name, EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p mode=%p", if_name, mode);
  (void)if_name;
  (void)mode;
  if (if_name == NULL || mode == NULL) {
    ESF_NETWORK_MANAGER_WARN("parameter error.(if_name=%p mode=%p)", if_name,
                             mode);
    return;
  }
  int ret = 0;
  if ((ret = dhcpd_start(if_name)) != 0) {
    ESF_NETWORK_MANAGER_WARN("dhcpd_start failed. ret=%d errno=%d", ret, errno);
    return;
  }
  mode->notify_info = kEsfNetworkManagerNotifyInfoApStart;
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeAccessPoint,
                                        mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

// """WiFi AP connected event reception processing during connection attempt.

// Increment the number of AP connections.
// Update led manager.
// Invoke callback function.

// Args:
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiApConnectedEventOnConnecting(
    EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%p", mode);
  if (mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mode=%p)", mode);
    return;
  }
  ++mode->ap_connected_count;
  mode->status_info.is_link_up = true;
  mode->connect_status = kEsfNetworkManagerConnectStatusConnected;
  mode->notify_info = kEsfNetworkManagerNotifyInfoConnected;
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          mode->connect_status, mode->notify_info);
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeAccessPoint,
                                        mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

// """WiFi AP connected event reception processing during connected.

// Increment the number of AP connections.

// Args:
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiApConnectedEventOnConnected(
    EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%p", mode);
  if (mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mode=%p)", mode);
    return;
  }
  ++mode->ap_connected_count;
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

// """WiFi AP disconnected event reception processing during connected.

// Decrement the number of AP connections.
// If the AP connection is lost, do the following.
// Update led manager.
// Invoke callback function.

// Args:
//     mode (const EsfNetworkManagerModeInfo *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiApDisconnectedEventOnConnected(
    EsfNetworkManagerModeInfo *mode) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%p", mode);
  if (mode == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mode=%p)", mode);
    return;
  }
  if (mode->ap_connected_count <= 0) {
    ESF_NETWORK_MANAGER_ERR("ap connected count failure. %d",
                            mode->ap_connected_count);
    return;
  }
  --mode->ap_connected_count;
  if (mode->ap_connected_count != 0) {
    return;
  }
  mode->status_info.is_link_up = false;
  mode->connect_status = kEsfNetworkManagerConnectStatusConnecting;
  mode->notify_info = kEsfNetworkManagerNotifyInfoDisconnected;
  ESF_NETWORK_MANAGER_DBG("UPDATE STATUS connect_status=%d notify_info=%d",
                          mode->connect_status, mode->notify_info);
  EsfNetworkManagerAccessorCallCallback(kEsfNetworkManagerModeAccessPoint,
                                        mode);
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

static void RecoveryDhcp(const char *if_name, EsfNetworkManagerModeInfo *mode) {
  // First, set IPAddr to 0
  EsfNetworkManagerInternalIPInfo ip_info = {
      .ip          = {0},
      .subnet_mask = {0},
      .gateway     = {0},
      .dns         = {0},
  };
  EsfNetworkManagerResult ret_network;

  ret_network = EsfNetworkManagerAccessorIpSetIpAddress(if_name, &ip_info,
                                                        false);
  if (ret_network != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("IP address set error.(if=%s, ret=%u)", if_name,
                            ret_network);
  }
  // To achieve the desired lighting pattern, disable high priority patterns.
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedNoInternetConnection, true);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingProxy, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingWithoutTLS, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingWithTLS, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusConnectedWithoutTLS, false);
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusConnectedWithTLS, false);
  ret_network = RequestDhcp(if_name, mode);
  if (ret_network != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Dhcp ip address get error.");
    return;
  }
  // The LED transition after “ConnectingDNSAndNTP” is handled by the EVP agent.
  // (This is the same as what NetworkManager does at startup.)
  EsfNetworkManagerAccessorSetLedManagerStatusService(
      kEsfLedManagerLedStatusDisconnectedConnectingDNSAndNTP, true);
  return;
}

static int RenewDhcp(const char *if_name, EsfNetworkManagerModeInfo *mode) {
  in_addr_t before_ip     = s_dhcp_status.ipaddr.s_addr;
  in_addr_t before_dns    = s_dhcp_status.dnsaddr.s_addr;
  in_addr_t before_mask   = s_dhcp_status.netmask.s_addr;
  in_addr_t before_router = s_dhcp_status.default_router.s_addr;

  int plret = PlNetworkDhcpcRenew(s_dhcp_handle, &s_dhcp_status);
  if (plret != 0) {
    ESF_NETWORK_MANAGER_ERR("dhcpc_renew error.(if=%s, ret=%d, errno=%d)",
                            if_name, plret, errno);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_DHCP_FAILURE);
    return errno;
  }
  s_dhcp_lease_time = s_dhcp_status.lease_time;
  s_dhcp_t1_time = s_dhcp_status.renewal_time;
  s_dhcp_t2_time = s_dhcp_status.rebinding_time;

  if (!IsValidDhcpTime()) {
    s_dhcp_t1_time = s_dhcp_lease_time * DHCP_RENEWAL_TIME_RATIO;
    s_dhcp_t2_time = s_dhcp_lease_time * DHCP_REBINDING_TIME_RATIO;
  }
  ESF_NETWORK_MANAGER_INFO("Lease:%" PRIu64 "[s] T1:%" PRIu64 "[s] T2:%" PRIu64
                           "[s]\n",
                           s_dhcp_lease_time, s_dhcp_t1_time, s_dhcp_t2_time);
  // Ip updated
  if ((before_ip     != s_dhcp_status.ipaddr.s_addr) ||
      (before_mask   != s_dhcp_status.netmask.s_addr) ||
      (before_router != s_dhcp_status.default_router.s_addr) ||
      (before_dns    != s_dhcp_status.dnsaddr.s_addr)) {
    EsfNetworkManagerInternalIPInfo ip_info = {
        .ip          = s_dhcp_status.ipaddr,
        .subnet_mask = s_dhcp_status.netmask,
        .gateway     = s_dhcp_status.default_router,
        .dns         = s_dhcp_status.dnsaddr,
    };
    EsfNetworkManagerResult ret;
    ret = EsfNetworkManagerAccessorIpSetIpAddress(if_name, &ip_info, false);
    if (ret != kEsfNetworkManagerResultSuccess) {
      ESF_NETWORK_MANAGER_ERR("IP address set error.(if=%s, ret=%u)", if_name,
                              ret);
    }
    if (ret == kEsfNetworkManagerResultSuccess) {
      ret = EsfNetworkManagerAccessorIpConvertToString(&ip_info,
                                                       &mode->ip_info);
      if (ret != kEsfNetworkManagerResultSuccess) {
        ESF_NETWORK_MANAGER_ERR("IP address save local error.(if=%s)", if_name);
      }
    }
  }

  struct timespec abstime;
  clock_gettime(CLOCK_MONOTONIC, &abstime);
  s_dhcp_update_time = abstime.tv_sec;
  ESF_NETWORK_MANAGER_INFO("DHCPC renew done. Next update:%" PRIu64 "[s] later",
                           s_dhcp_t1_time);
  return 0;
}

static void EsfNetworkManagerAccessorLeaseEvent(
    const char *if_name, EsfNetworkManagerModeInfo *mode_info) {
  PlNetworkCapabilities capabilities = PlNetworkGetCapabilities();
  if (capabilities.use_external_dhcpc) {
    return;
  }
  bool is_if_up = mode_info->status_info.is_if_up;
  bool is_link_up = mode_info->status_info.is_link_up;
  bool is_dhcp = (mode_info->connect_info.normal_mode.ip_method ==
                  kEsfNetworkManagerIpMethodDhcp);
  if ((is_if_up && is_link_up && is_dhcp) == false) {
    return;
  }

  uint64_t elapsed_time = 0;
  struct timespec abstime;
  clock_gettime(CLOCK_MONOTONIC, &abstime);
  elapsed_time = abstime.tv_sec - s_dhcp_update_time;

  if (s_dhcp_lease_time < elapsed_time) {
    ESF_NETWORK_MANAGER_INFO("exec DHCPC recovery.");
    RecoveryDhcp(if_name, mode_info);
    return;
  }

  uint64_t *fail_time = NULL;
  if (s_dhcp_t2_time < elapsed_time) {
    fail_time = &s_t2_fail_time;
    if (!ShouldRenewDhcp(abstime.tv_sec, *fail_time)) {
      return;
    }
    s_dhcp_status.serverid.s_addr = INADDR_BROADCAST;
    ESF_NETWORK_MANAGER_INFO("DHCP T2 passed. DHCPC renew.");
  } else if (s_dhcp_t1_time < elapsed_time) {
    fail_time = &s_t1_fail_time;
    if (!ShouldRenewDhcp(abstime.tv_sec, *fail_time)) {
      return;
    }
    ESF_NETWORK_MANAGER_INFO("DHCP T1 passed. DHCPC renew.");
  } else {
    // Do not renew.
    return;
  }

  *fail_time = 0;
  int ret = RenewDhcp(if_name, mode_info);
  if (ret == ECONNREFUSED) {
    // goto recovery
    ESF_NETWORK_MANAGER_INFO("DHCPC recv NAK.");
    s_dhcp_lease_time = 0;
    s_dhcp_t1_time = 0;
    s_dhcp_t2_time = 0;
    s_dhcp_update_time = 0;
    s_t1_fail_time = 0;
    s_t2_fail_time = 0;
  } else if ((ret == ETIMEDOUT) || (ret == EAGAIN) || (ret == EINTR)) {
    *fail_time = abstime.tv_sec;
  } else {
    // Do DHCPC renew after 1 second.
  }

  return;
}

static void EsfNetworkManagerAccessorIfDownPreEventOnDisconnecting(void) {
  WaitEventHandle *h = &s_if_down_pre_event_handle;
  int ret = 0;
  if (s_dhcp_handle == NULL) {
    goto finally;
  }
  ret = PlNetworkDhcpcRelease(s_dhcp_handle, &s_dhcp_status);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_WARN("PlNetworkDhcpcRelease:%d:%d", ret, errno);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_DHCP_FAILURE);
    goto finally;
  }
  ESF_NETWORK_MANAGER_INFO("DHCPC release done.");

finally:
  CompleteEvent(h);
  return;
}

// """HAL event reception processing for connection type Ether.

// Necessary processing is carried out for each connection state.

// Args:
//     if_name (const char *): The interface on which the event occurred.
//     event (PlNetworkEvent): The type of event that occurred.
//     private_data (void *): Network resource information.
// Note:
// """
static void EsfNetworkManagerAccessorEtherEventHandler(const char *if_name,
                                                       PlNetworkEvent event,
                                                       void *private_data) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p event=%d private_data=%p",
                            if_name, event, private_data);
  if (if_name == NULL || private_data == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p private_data=%p)",
                            if_name, private_data);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return;
  }
  ESF_NETWORK_MANAGER_DBG(
      "Ether event receive if_name=%s event=%d private_data=%p", if_name, event,
      private_data);

  // IfDownPreEvent is processed before resource locking to prevent deadlocks
  if (event == kPlNetworkEventIfDownPre) {
    EsfNetworkManagerAccessorIfDownPreEventOnDisconnecting();
    return;
  }

  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.(ret=%u)", ret);
    return;
  }
  do {
    if (!EsfNetworkManagerResourceCheckStatus(kEsfNetworkManagerStatusInit)) {
      ESF_NETWORK_MANAGER_ERR("Not initialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      break;
    }
    EsfNetworkManagerModeInfo *mode_info =
        (EsfNetworkManagerModeInfo *)private_data;
    switch (event) {
      case kPlNetworkEventIfUp:
        mode_info->status_info.is_if_up = true;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorEtherIfLinkupEventOnConnecting(if_name,
                                                                  mode_info);
        }
        break;
      case kPlNetworkEventLinkUp:
        mode_info->status_info.is_link_up = true;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorEtherIfLinkupEventOnConnecting(if_name,
                                                                  mode_info);
        }
        break;
      case kPlNetworkEventIfDown:
        mode_info->status_info.is_if_up = false;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnected) {
          EsfNetworkManagerAccessorEtherLinkdownEventOnConnected(mode_info);
        } else if (mode_info->connect_status ==
                   kEsfNetworkManagerConnectStatusDisconnecting) {
          EsfNetworkManagerAccessorEtherIfDownEventOnDisconnecting(mode_info);
        }
        ESF_NETWORK_MANAGER_INFO("Ether %s is disconnected.", if_name);
        break;
      case kPlNetworkEventLinkDown:
        mode_info->status_info.is_link_up = false;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnected) {
          EsfNetworkManagerAccessorEtherLinkdownEventOnConnected(mode_info);
          ESF_NETWORK_MANAGER_INFO("Ether %s is disconnected.", if_name);
        }
        break;
      case kPlNetworkEvent1sec:
        // check lease_time every 1sec.
        EsfNetworkManagerAccessorLeaseEvent(if_name, mode_info);
        break;
      default:
        ESF_NETWORK_MANAGER_DBG("event discarded %u", event);
        break;
    }
  } while (0);
  ret = EsfNetworkManagerResourceUnlockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

// """HAL event reception processing for connection type Wifi ST.

// Necessary processing is carried out for each connection state.

// Args:
//     if_name (const char *): The interface on which the event occurred.
//     event (PlNetworkEvent): The type of event that occurred.
//     private_data (void *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiStationEventHandler(
    const char *if_name, PlNetworkEvent event, void *private_data) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p event=%d private_data=%p",
                            if_name, event, private_data);
  if (if_name == NULL || private_data == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p private_data=%p)",
                            if_name, private_data);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return;
  }
  ESF_NETWORK_MANAGER_DBG(
      "WiFi station event receive if_name=%s event=%d private_data=%p", if_name,
      event, private_data);

  // IfDownPreEvent is processed before resource locking to prevent deadlocks
  if (event == kPlNetworkEventIfDownPre) {
    EsfNetworkManagerAccessorIfDownPreEventOnDisconnecting();
    return;
  }

  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.(ret=%u)", ret);
    return;
  }
  ESF_NETWORK_MANAGER_TRACE("Locked");
  do {
    if (!EsfNetworkManagerResourceCheckStatus(kEsfNetworkManagerStatusInit)) {
      ESF_NETWORK_MANAGER_ERR("Not initialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      break;
    }
    EsfNetworkManagerModeInfo *mode_info =
        (EsfNetworkManagerModeInfo *)private_data;
    switch (event) {
      case kPlNetworkEventWifiStaConnected:
        mode_info->status_info.is_link_up = true;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorWifiStationConnectedEventOnConnecting(
              if_name, mode_info);
        }
        break;
      case kPlNetworkEventWifiStaDisconnected:
        mode_info->status_info.is_link_up = false;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnected) {
          EsfNetworkManagerAccessorWifiStationDisconnectedEventOnConnected(
              if_name, mode_info);
        } else if (mode_info->connect_status ==
                   kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorWifiStationDisconnectedEventOnConnecting(
              if_name, mode_info);
        }
        ESF_NETWORK_MANAGER_INFO("WiFi %s is disconnected.", if_name);
        break;
      case kPlNetworkEventWifiStaStop:
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorWifiStationStopEventOnConnecting(if_name,
                                                                    mode_info);
        }
        ESF_NETWORK_MANAGER_INFO("WiFi %s is stopped.", if_name);
        break;
      case kPlNetworkEvent1sec:
        // check lease_time every 1sec.
        EsfNetworkManagerAccessorLeaseEvent(if_name, mode_info);
        break;
      default:
        ESF_NETWORK_MANAGER_DBG("event discarded %u", event);
        break;
    }
  } while (0);
  ret = EsfNetworkManagerResourceUnlockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_STA_MODE_DISABLE

// """HAL event reception processing for connection type Wifi AP.

// Necessary processing is carried out for each connection state.

// Args:
//     if_name (const char *): The interface on which the event occurred.
//     event (PlNetworkEvent): The type of event that occurred.
//     private_data (void *): Network resource information.
// Note:
// """
#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
static void EsfNetworkManagerAccessorWifiApEventHandler(const char *if_name,
                                                        PlNetworkEvent event,
                                                        void *private_data) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p event=%d private_data=%p",
                            if_name, event, private_data);
  if (if_name == NULL || private_data == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p private_data=%p)",
                            if_name, private_data);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
    return;
  }
  EsfNetworkManagerResult ret = EsfNetworkManagerResourceLockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource lock error.(ret=%u)", ret);
    return;
  }
  do {
    if (!EsfNetworkManagerResourceCheckStatus(kEsfNetworkManagerStatusInit)) {
      ESF_NETWORK_MANAGER_ERR("Not initialized.");
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      break;
    }
    EsfNetworkManagerModeInfo *mode_info =
        (EsfNetworkManagerModeInfo *)private_data;
    switch (event) {
      case kPlNetworkEventWifiApStart:
        mode_info->status_info.is_if_up = true;
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorWifiApStartEventOnConnecting(if_name,
                                                                mode_info);
        }
        break;
      case kPlNetworkEventWifiApConnected:
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnecting) {
          EsfNetworkManagerAccessorWifiApConnectedEventOnConnecting(mode_info);
        } else if (mode_info->connect_status ==
                   kEsfNetworkManagerConnectStatusConnected) {
          EsfNetworkManagerAccessorWifiApConnectedEventOnConnected(mode_info);
        }
        break;
      case kPlNetworkEventWifiApDisconnected:
        if (mode_info->connect_status ==
            kEsfNetworkManagerConnectStatusConnected) {
          EsfNetworkManagerAccessorWifiApDisconnectedEventOnConnected(
              mode_info);
        }
        break;
      default:
        ESF_NETWORK_MANAGER_DBG("event discarded %u", event);
        break;
    }
  } while (0);
  ret = EsfNetworkManagerResourceUnlockResource();
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("Resource unlock error.");
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}
#endif  // #ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE

EsfNetworkManagerResult EsfNetworkManagerAccessorGetSystemInfo(
    uint32_t *info_total_num, PlNetworkSystemInfo **infos) {
  ESF_NETWORK_MANAGER_TRACE("START info_total_num=%p infos=%p", info_total_num,
                            infos);
  if (info_total_num == NULL || infos == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(info_total_num=%p infos=%p)",
                            info_total_num, infos);
    return kEsfNetworkManagerResultInternalError;
  }
  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_GET_SYSTEM_INFO(info_total_num, infos);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("HAL(PlNetworkGetSystemInfo) error.(ret=%u)",
                            hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  ESF_NETWORK_MANAGER_DBG("PL system information.");
  EsfNetworkManagerShowPlSystemInfo(*info_total_num, *infos);
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerAccessorGetIFInfo(
    const EsfNetworkManagerModeInfo *mode, EsfNetworkManagerIPInfo *ip_info) {
  PlNetworkCapabilities capabilities = PlNetworkGetCapabilities();
  if (!capabilities.use_external_dhcpc) {
    *ip_info = mode->ip_info;
    return kEsfNetworkManagerResultSuccess;
  }
  ESF_NETWORK_MANAGER_TRACE("START mode=%p", mode);
  EsfNetworkManagerInterfaceKind kind = kEsfNetworkManagerInterfaceKindWifi;
  if (mode->connect_info.normal_mode.netif_kind) {
    kind = kEsfNetworkManagerInterfaceKindEther;
  }
  const char *if_name = mode->hal_system_info[kind]->if_name;
  EsfNetworkManagerInternalIPInfo wk = {0};
  int ret = 0;
  ret = PlNetworkGetIpv4Addr(if_name, &wk.ip, &wk.subnet_mask);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkGetIpv4Addr error.(ret=%d)", ret);
    return kEsfNetworkManagerResultUtilityIPAddressError;
  }
  ret = PlNetworkGetIpv4Gateway(if_name, &wk.gateway);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkGetIpv4Gateway error.(ret=%d)", ret);
    return kEsfNetworkManagerResultUtilityIPAddressError;
  }
  ret = PlNetworkGetIpv4Dns(if_name, &wk.dns);
  if (ret != 0) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkGetIpv4Dns error.(ret=%d)", ret);
    return kEsfNetworkManagerResultUtilityIPAddressError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return EsfNetworkManagerAccessorIpConvertToString(&wk, ip_info);
}

EsfNetworkManagerResult EsfNetworkManagerAccessorGetStatusInfo(
    const char *if_name, EsfNetworkManagerStatusInfo *status) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p status=%p", if_name, status);
  if (if_name == NULL || status == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p status=%p)", if_name,
                            status);
    return kEsfNetworkManagerResultInternalError;
  }
  PlNetworkStatus pl_status = {
      .is_link_up = false,
      .is_if_up = false,
  };
  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_GET_STATUS(if_name, &pl_status);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("HAL(PlNetworkGetStatus) error.(ret=%u)", hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  status->is_link_up = pl_status.is_link_up;
  status->is_if_up = pl_status.is_if_up;
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerAccessorGetRssi(const char *if_name,
                                                         int8_t *rssi) {
  ESF_NETWORK_MANAGER_TRACE("START if_name=%p rssi=%p", if_name, rssi);
  if (if_name == NULL || rssi == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(if_name=%p rssi=%p)", if_name,
                            rssi);
    return kEsfNetworkManagerResultInternalError;
  }
  PlNetworkStatus pl_status = {
      .is_if_up = false,
  };
  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_GET_STATUS(if_name, &pl_status);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("HAL(PlNetworkGetStatus) error.(ret=%u)", hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  *rssi = pl_status.network.wifi.rssi;
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

EsfNetworkManagerResult EsfNetworkManagerAccessorGetNetstat(
    const int32_t netstat_buf_size, char *netstat_buf) {
  ESF_NETWORK_MANAGER_TRACE("START netstat_buf_size=%d netstat_buf=%p",
                            netstat_buf_size, netstat_buf);
  if (netstat_buf_size <= 0 || netstat_buf == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(netstat_buf_size=%d netstat_buf=%p)",
        netstat_buf_size, netstat_buf);
    return kEsfNetworkManagerResultInternalError;
  }
  PlErrCode hal_ret = kPlErrCodeOk;
  hal_ret = NETWORK_MANAGER_PL_NETWORK_GET_NET_STAT(netstat_buf,
                                                    netstat_buf_size);
  if (hal_ret != kPlErrCodeOk) {
    ESF_NETWORK_MANAGER_ERR("HAL(PlNetworkGetNetStat) error.(ret=%u)", hal_ret);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

void EsfNetworkManagerAccessorUnregisterAllEventHandler(
    const char *if_name_ether, const char *if_name_wifist,
    const char *if_name_wifiap) {
  ESF_NETWORK_MANAGER_TRACE(
      "START if_name_ether=%p if_name_wifist=%p if_name_wifiap=%p",
      if_name_ether, if_name_wifist, if_name_wifiap);
  PlErrCode hal_ret = kPlErrCodeOk;
  if (if_name_ether != NULL) {
    ESF_NETWORK_MANAGER_DBG("Unregister if_name_ether=%s", if_name_ether);
    hal_ret =
        NETWORK_MANAGER_PL_NETWORK_UNREGISTER_EVENT_HANDLER(if_name_ether);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG(
          "HAL(PlNetworkUnregisterEventHandler) error.(if=%s ret=%d)",
          if_name_ether, hal_ret);
      // Ignore errors.
    }
  }
  if (if_name_wifist != NULL) {
    ESF_NETWORK_MANAGER_DBG("Unregister if_name_wifist=%s", if_name_wifist);
    hal_ret =
        NETWORK_MANAGER_PL_NETWORK_UNREGISTER_EVENT_HANDLER(if_name_wifist);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG(
          "HAL(PlNetworkUnregisterEventHandler) error.(if=%s ret=%d)",
          if_name_wifist, hal_ret);
      // Ignore errors.
    }
  }
  if (if_name_wifiap != NULL) {
    ESF_NETWORK_MANAGER_DBG("Unregister if_name_wifiap=%s", if_name_wifiap);
    hal_ret =
        NETWORK_MANAGER_PL_NETWORK_UNREGISTER_EVENT_HANDLER(if_name_wifiap);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG(
          "HAL(PlNetworkUnregisterEventHandler) error.(if=%s ret=%d)",
          if_name_wifiap, hal_ret);
      // Ignore errors.
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

void EsfNetworkManagerAccessorStopAllNetwork(const char *if_name_ether,
                                             const char *if_name_wifist,
                                             const char *if_name_wifiap) {
  ESF_NETWORK_MANAGER_TRACE(
      "START if_name_ether=%p if_name_wifist=%p if_name_wifiap=%p",
      if_name_ether, if_name_wifist, if_name_wifiap);
  PlErrCode hal_ret = kPlErrCodeOk;
  if (if_name_ether != NULL) {
    ESF_NETWORK_MANAGER_DBG("Stop if_name_ether=%s", if_name_ether);
    hal_ret = NETWORK_MANAGER_PL_NETWORK_STOP(if_name_ether);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG("HAL(PlNetworkStop) error.(if=%s ret=%d)",
                              if_name_ether, hal_ret);
      // Ignore errors.
    }
  }
  if (if_name_wifist != NULL) {
    ESF_NETWORK_MANAGER_DBG("Stop if_name_wifist=%s", if_name_wifist);
    hal_ret = NETWORK_MANAGER_PL_NETWORK_STOP(if_name_wifist);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG("HAL(PlNetworkStop) error.(if=%s ret=%d)",
                              if_name_wifist, hal_ret);
      // Ignore errors.
    }
  }
  if (if_name_wifiap != NULL) {
    ESF_NETWORK_MANAGER_DBG("Stop if_name_wifiap=%s", if_name_wifiap);
    hal_ret = NETWORK_MANAGER_PL_NETWORK_STOP(if_name_wifiap);
    if (hal_ret != kPlErrCodeOk) {
      ESF_NETWORK_MANAGER_DBG("HAL(PlNetworkStop) error.(if=%s ret=%d)",
                              if_name_wifiap, hal_ret);
      // Ignore errors.
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return;
}

EsfNetworkManagerResult EsfNetworkManagerAccessorStart(
    const EsfNetworkManagerOSInfo *connect_info,
    EsfNetworkManagerHandleInternal *handle_internal) {
  ESF_NETWORK_MANAGER_TRACE("START connect_info=%p handle_internal=%p",
                            connect_info, handle_internal);
  if (connect_info == NULL || handle_internal == NULL) {
    ESF_NETWORK_MANAGER_ERR(
        "parameter error.(connect_info=%p handle_internal=%p)", connect_info,
        handle_internal);
    return kEsfNetworkManagerResultInternalError;
  }
  EsfNetworkManagerResult ret = kEsfNetworkManagerResultSuccess;
  switch (handle_internal->mode) {
    case kEsfNetworkManagerModeNormal:
      ret = EsfNetworkManagerAccessorNormalStart(connect_info, handle_internal);
      break;
    case kEsfNetworkManagerModeAccessPoint:
      ret = EsfNetworkManagerAccessorWifiApStart(connect_info, handle_internal);
      break;
    default:
      ESF_NETWORK_MANAGER_ERR("parameter error.(mode=%u)",
                              handle_internal->mode);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INVALID_PARAM2);
      ret = kEsfNetworkManagerResultInternalError;
      break;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return ret;
}

EsfNetworkManagerResult EsfNetworkManagerAccessorStop(
    EsfNetworkManagerHandleInternal *handle_internal) {
  ESF_NETWORK_MANAGER_TRACE("START handle_internal=%p", handle_internal);
  if (handle_internal == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(handle_internal=%p)",
                            handle_internal);
    return kEsfNetworkManagerResultInternalError;
  }

  EsfNetworkManagerInterfaceKind interface_kind =
      kEsfNetworkManagerInterfaceKindEther;
  if (handle_internal->mode_info->connect_info.normal_mode.netif_kind ==
      kEsfNetworkManagerNetifKindWiFi) {
    interface_kind = kEsfNetworkManagerInterfaceKindWifi;
  }

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_WIFI_AP_MODE_DISABLE
  if (handle_internal->mode == kEsfNetworkManagerModeAccessPoint) {
    int dhcpd_ret = dhcpd_stop();
    if (dhcpd_ret != 0) {
      ESF_NETWORK_MANAGER_WARN("dhcpd_stop failed. ret=%d errno=%d", dhcpd_ret,
                               errno);
      return kEsfNetworkManagerResultUtilityDHCPServerError;
    }
  }
#endif
  PlErrCode hal_ret = kPlErrCodeOk;
  const char *if_name =
      handle_internal->mode_info->hal_system_info[interface_kind]->if_name;

  // PlNetworkStopPre sends IfDownPreEvent from pl to esf
  // IfDownPreEvent is performed to "DHCP Release" before ifdown
  WaitEventHandle *h = &s_if_down_pre_event_handle;
  WaitEventHandleInit(h);
  hal_ret = PlNetworkStopPre(if_name);
  if (hal_ret == kPlErrCodeOk) {
    WaitEvent(h);
  }
  WaitEventHandleDeInit(h);

  hal_ret = NETWORK_MANAGER_PL_NETWORK_STOP(if_name);
  // A stopped response is treated as a success.
  if (hal_ret != kPlErrCodeOk && hal_ret != kPlErrInvalidState) {
    ESF_NETWORK_MANAGER_ERR("PlNetworkStop failed. ret=%u errno=%d", hal_ret,
                            errno);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
    return kEsfNetworkManagerResultHWIFError;
  }
  ESF_NETWORK_MANAGER_TRACE("END");
  return kEsfNetworkManagerResultSuccess;
}

void EsfNetworkManagerAccessorSetLedManagerStatus(
    EsfNetworkManagerMode mode,
    EsfNetworkManagerAccessorLedManagerStatus status,
    EsfNetworkManagerNetifKind netif_kind) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%d status=%d netif_kind=%d", mode,
                            status, netif_kind);
  if (mode != kEsfNetworkManagerModeNormal ||
      netif_kind != kEsfNetworkManagerNetifKindWiFi) {
    return;
  }
  EsfLedManagerResult ret_led_manager = kEsfLedManagerSuccess;
  EsfLedManagerLedStatusInfo loading_ssid_status = {
      kEsfLedManagerTargetLedWifi, kEsfLedManagerLedStatusLoadingSSIDPassword,
      false};
  EsfLedManagerLedStatusInfo ap_authentication_status = {
      kEsfLedManagerTargetLedWifi,
      kEsfLedManagerLedStatusAPFoundAndDoingAuthentication, false};
  EsfLedManagerLedStatusInfo link_status = {
      kEsfLedManagerTargetLedWifi, kEsfLedManagerLedStatusLinkEstablished,
      false};

  switch (status) {
    case kEsfNetworkManagerAccessorLedManagerStatusInitializeError:
    case kEsfNetworkManagerAccessorLedManagerStatusDeinitializeError:
    case kEsfNetworkManagerAccessorLedManagerStatusStartError:
    case kEsfNetworkManagerAccessorLedManagerStatusStopError:
    case kEsfNetworkManagerAccessorLedManagerStatusParameterError:
      // No LED operation.
      return;
    case kEsfNetworkManagerAccessorLedManagerStatusLoadingSSID:
      loading_ssid_status.enabled = true;
      break;
    case kEsfNetworkManagerAccessorLedManagerStatusConnecting:
      ap_authentication_status.enabled = true;
      break;
    case kEsfNetworkManagerAccessorLedManagerStatusConnected:
      link_status.enabled = true;
      break;
    case kEsfNetworkManagerAccessorLedManagerStatusDisconnecting:
      // Nothing due to invalid settings.
      break;
    case kEsfNetworkManagerAccessorLedManagerStatusDisconnected:
      // Nothing due to invalid settings.
      break;
    case kEsfNetworkManagerAccessorLedManagerStatusIgnore:
      // No led_manager operation is performed.
      ESF_NETWORK_MANAGER_TRACE("END");
      return;
    default:
      ESF_NETWORK_MANAGER_ERR("Input parameter error.(status=%u)", status);
      return;
  }
  ESF_NETWORK_MANAGER_DBG("Led Set Led:%d Status:%d Enabled:%d",
                          link_status.led, link_status.status,
                          link_status.enabled);
  ret_led_manager = EsfLedManagerSetStatus(&link_status);
  if (ret_led_manager != kEsfLedManagerSuccess) {
    ESF_NETWORK_MANAGER_ERR("LedManager set status error.(ret=%u)",
                            ret_led_manager);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_LED_FAILURE);
  }
  ESF_NETWORK_MANAGER_DBG(
      "Led Set Led:%d Status:%d Enabled:%d", ap_authentication_status.led,
      ap_authentication_status.status, ap_authentication_status.enabled);
  ret_led_manager = EsfLedManagerSetStatus(&ap_authentication_status);
  if (ret_led_manager != kEsfLedManagerSuccess) {
    ESF_NETWORK_MANAGER_ERR("LedManager set status error.(ret=%u)",
                            ret_led_manager);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_LED_FAILURE);
  }
  ESF_NETWORK_MANAGER_DBG("Led Set Led:%d Status:%d Enabled:%d",
                          loading_ssid_status.led, loading_ssid_status.status,
                          loading_ssid_status.enabled);
  ret_led_manager = EsfLedManagerSetStatus(&loading_ssid_status);
  if (ret_led_manager != kEsfLedManagerSuccess) {
    ESF_NETWORK_MANAGER_ERR("LedManager set status error.(ret=%u)",
                            ret_led_manager);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_LED_FAILURE);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

void EsfNetworkManagerAccessorSetLedManagerStatusService(
    EsfLedManagerLedStatus status, bool enabled) {
  ESF_NETWORK_MANAGER_TRACE("START status=%u enabled=%u", status,
                            enabled);
  EsfLedManagerResult ret_led_manager = kEsfLedManagerSuccess;
  EsfLedManagerLedStatusInfo set_led_status = {kEsfLedManagerTargetLedService,
                                               status, enabled};

  ESF_NETWORK_MANAGER_DBG("Led Set Led:%u Status:%u Enabled:%u",
                          set_led_status.led, set_led_status.status,
                          set_led_status.enabled);
  ret_led_manager = EsfLedManagerSetStatus(&set_led_status);
  if (ret_led_manager != kEsfLedManagerSuccess) {
    ESF_NETWORK_MANAGER_ERR("LedManager set status error.(ret=%u)",
                            ret_led_manager);
    ESF_NETWORK_MANAGER_ELOG_WARN(ESF_NETWORK_MANAGER_ELOG_LED_FAILURE);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE

void EsfNetworkManagerAccessorCallCallback(
    EsfNetworkManagerMode mode, const EsfNetworkManagerModeInfo *mode_info) {
  ESF_NETWORK_MANAGER_TRACE("START mode=%d mode_info=%p", mode, mode_info);
  if (mode_info == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mode_info=%p)", mode_info);
    return;
  }
  if (mode_info->callback != NULL) {
    mode_info->callback(mode, mode_info->notify_info,
                        mode_info->callback_private_data);
    ESF_NETWORK_MANAGER_DBG("Callback call mode=%d info=%d private_data=%p",
                            mode, mode_info->notify_info,
                            mode_info->callback_private_data);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
void EsfNetworkManagerAccessorInit(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  PlErrCode pl_error = NETWORK_MANAGER_PL_NETWORK_INITIALIZE();
  if (pl_error != kPlErrCodeOk) {
    // Nothing to do.
    ESF_NETWORK_MANAGER_ERR("PlNetworkInitialize error %u.", pl_error);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

void EsfNetworkManagerAccessorDeinit(void) {
  ESF_NETWORK_MANAGER_TRACE("START");
  PlErrCode pl_error = NETWORK_MANAGER_PL_NETWORK_FINALIZE();
  if (pl_error != kPlErrCodeOk) {
    // Nothing to do.
    ESF_NETWORK_MANAGER_ERR("PlNetworkFinalize error %u.", pl_error);
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_PL_FAILURE);
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}
#endif  // CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
