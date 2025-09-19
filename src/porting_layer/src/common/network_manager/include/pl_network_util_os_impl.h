/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_NETWORK_UTIL_OS_IMPL_H_
#define PL_NETWORK_UTIL_OS_IMPL_H_

// Includes --------------------------------------------------------------------
#include <stdint.h>

#include "pl_network.h"
#include "pl_network_util.h"

// Public functions ------------------------------------------------------------
PlNetworkUtilIrqstate PlNetworkUtilNetworkLockOsImpl(void);
void PlNetworkUtilNetworkUnlockOsImpl(PlNetworkUtilIrqstate flags);
void PlNetworkUtilSchedLockOsImpl(void);
void PlNetworkUtilSchedUnlockOsImpl(void);
PlErrCode PlNetworkUtilGetNetStatOsImpl(char *buf, const uint32_t buf_size);
PlNetworkCapabilities PlNetworkGetCapabilitiesOsImpl(void);
PlErrCode PlNetworkSetIfStatusOsImpl(const char *ifname, const bool is_ifup);
PlErrCode PlNetworkGetIfStatusOsImpl(const char *ifname, bool *is_ifup);
PlErrCode PlNetworkGetLinkStatusOsImpl(const char *ifname, bool *is_linkup,
                                       bool *is_phy_id_valid);
int PlNetworkSetIpv4AddrOsImpl(const char *ifname, const struct in_addr *addr,
                               const struct in_addr *mask);
int PlNetworkSetIpv4GatewayOsImpl(const char *ifname,
                                  const struct in_addr *addr);
int PlNetworkSetIpv4DnsOsImpl(const char *ifname, const struct in_addr *addr);
int PlNetworkSetIpv4MethodOsImpl(const char *ifname, bool is_dhcp);
int PlNetworkGetIpv4AddrOsImpl(const char *ifname, struct in_addr *addr,
                               struct in_addr *mask);
int PlNetworkGetIpv4GatewayOsImpl(const char *ifname, struct in_addr *addr);
int PlNetworkGetIpv4DnsOsImpl(const char *ifname, struct in_addr *addr);
int PlNetworkGetMacAddrOsImpl(const char *ifname, uint8_t *mac);
void *PlNetworkDhcpcOpenOsImpl(const char *ifname, void *mac, int maclen);
int PlNetworkDhcpcRequestOsImpl(void *handle,
                                struct PlNetworkDhcpcState *state);
int PlNetworkDhcpcRenewOsImpl(void *handle, struct PlNetworkDhcpcState *state);
int PlNetworkDhcpcReleaseOsImpl(void *handle,
                                struct PlNetworkDhcpcState *state);
void PlNetworkDhcpcCloseOsImpl(void *handle);
int PlNetworkResetDnsServerOsImpl(void);

#endif  // PL_NETWORK_UTIL_OS_IMPL_H_
