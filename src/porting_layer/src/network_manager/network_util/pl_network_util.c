/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pl_network_util.h"

#include "pl_network_util_impl.h"

PlNetworkCapabilities PlNetworkGetCapabilities(void) {
  return PlNetworkGetCapabilitiesImpl();
}

PlErrCode PlNetworkSetIfStatus(const char *ifname, const bool is_ifup) {
  return PlNetworkSetIfStatusImpl(ifname, is_ifup);
}

PlErrCode PlNetworkGetIfStatus(const char *ifname, bool *is_ifup) {
  return PlNetworkGetIfStatusImpl(ifname, is_ifup);
}

PlErrCode PlNetworkGetLinkStatus(const char *ifname, bool *is_linkup,
                                 bool *is_phy_id_valid) {
  return PlNetworkGetLinkStatusImpl(ifname, is_linkup, is_phy_id_valid);
}

int PlNetworkSetIpv4Addr(const char *ifname, const struct in_addr *addr,
                         const struct in_addr *mask) {
  return PlNetworkSetIpv4AddrImpl(ifname, addr, mask);
}

int PlNetworkSetIpv4Gateway(const char *ifname, const struct in_addr *addr) {
  return PlNetworkSetIpv4GatewayImpl(ifname, addr);
}

int PlNetworkSetIpv4Dns(const char *ifname, const struct in_addr *addr) {
  return PlNetworkSetIpv4DnsImpl(ifname, addr);
}

int PlNetworkSetIpv4Method(const char *ifname, bool is_dhcp) {
  return PlNetworkSetIpv4MethodImpl(ifname, is_dhcp);
}

int PlNetworkGetIpv4Addr(const char *ifname, struct in_addr *addr,
                         struct in_addr *mask) {
  return PlNetworkGetIpv4AddrImpl(ifname, addr, mask);
}

int PlNetworkGetIpv4Gateway(const char *ifname, struct in_addr *addr) {
  return PlNetworkGetIpv4GatewayImpl(ifname, addr);
}

int PlNetworkGetIpv4Dns(const char *ifname, struct in_addr *addr) {
  return PlNetworkGetIpv4DnsImpl(ifname, addr);
}

int PlNetworkGetMacAddr(const char *ifname, uint8_t *mac) {
  return PlNetworkGetMacAddrImpl(ifname, mac);
}

void *PlNetworkDhcpcOpen(const char *ifname, void *mac, int maclen) {
  return PlNetworkDhcpcOpenImpl(ifname, mac, maclen);
}

int PlNetworkDhcpcRequest(void *handle, struct PlNetworkDhcpcState *state) {
  return PlNetworkDhcpcRequestImpl(handle, state);
}

int PlNetworkDhcpcRenew(void *handle, struct PlNetworkDhcpcState *state) {
  return PlNetworkDhcpcRenewImpl(handle, state);
}

void PlNetworkDhcpcClose(void *handle) {
  return PlNetworkDhcpcCloseImpl(handle);
}

int PlNetworkResetDnsServer(void) {
  return PlNetworkResetDnsServerImpl();
}
