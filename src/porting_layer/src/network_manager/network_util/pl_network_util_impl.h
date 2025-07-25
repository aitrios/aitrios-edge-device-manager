/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_IMPL_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_IMPL_H_

#include "pl_network.h"

PlNetworkCapabilities PlNetworkGetCapabilitiesImpl(void);
PlErrCode PlNetworkSetIfStatusImpl(const char *ifname, const bool is_ifup);
PlErrCode PlNetworkGetIfStatusImpl(const char *ifname, bool *is_ifup);
PlErrCode PlNetworkGetLinkStatusImpl(const char *ifname, bool *is_linkup,
                                     bool *is_phy_id_valid);
int PlNetworkSetIpv4AddrImpl(const char *ifname, const struct in_addr *addr,
                             const struct in_addr *mask);
int PlNetworkSetIpv4GatewayImpl(const char *ifname, const struct in_addr *addr);
int PlNetworkSetIpv4DnsImpl(const char *ifname, const struct in_addr *addr);
int PlNetworkSetIpv4MethodImpl(const char *ifname, bool is_dhcp);
int PlNetworkGetIpv4AddrImpl(const char *ifname, struct in_addr *addr,
                             struct in_addr *mask);
int PlNetworkGetIpv4GatewayImpl(const char *ifname, struct in_addr *addr);
int PlNetworkGetIpv4DnsImpl(const char *ifname, struct in_addr *addr);
int PlNetworkGetMacAddrImpl(const char *ifname, uint8_t *mac);
void *PlNetworkDhcpcOpenImpl(const char *ifname, void *mac, int maclen);
int PlNetworkDhcpcRequestImpl(void *handle, struct PlNetworkDhcpcState *state);
int PlNetworkDhcpcRenewImpl(void *handle, struct PlNetworkDhcpcState *state);
void PlNetworkDhcpcCloseImpl(void *handle);
int PlNetworkResetDnsServerImpl(void);

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_IMPL_H_
