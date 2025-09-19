/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_network_util.h"

#include <stdint.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_util_os_impl.h"

// Public functions ------------------------------------------------------------
PlNetworkUtilIrqstate PlNetworkUtilNetworkLock(void) {
  return PlNetworkUtilNetworkLockOsImpl();
}

// -----------------------------------------------------------------------------
void PlNetworkUtilNetworkUnlock(PlNetworkUtilIrqstate flags) {
  PlNetworkUtilNetworkUnlockOsImpl(flags);
}

// -----------------------------------------------------------------------------
void PlNetworkUtilSchedLock(void) {
  PlNetworkUtilSchedLockOsImpl();
  return;
}

// -----------------------------------------------------------------------------
void PlNetworkUtilSchedUnlock(void) {
  PlNetworkUtilSchedUnlockOsImpl();
  return;
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkUtilGetNetStat(char *buf, const uint32_t buf_size) {
  return PlNetworkUtilGetNetStatOsImpl(buf, buf_size);
}

// -----------------------------------------------------------------------------
PlNetworkCapabilities PlNetworkGetCapabilities(void) {
  return PlNetworkGetCapabilitiesOsImpl();
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkSetIfStatus(const char *ifname, const bool is_ifup) {
  return PlNetworkSetIfStatusOsImpl(ifname, is_ifup);
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetIfStatus(const char *ifname, bool *is_ifup) {
  return PlNetworkGetIfStatusOsImpl(ifname, is_ifup);
}

// -----------------------------------------------------------------------------
PlErrCode PlNetworkGetLinkStatus(const char *ifname, bool *is_linkup,
                                 bool *is_phy_id_valid) {
  return PlNetworkGetLinkStatusOsImpl(ifname, is_linkup, is_phy_id_valid);
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4Addr(const char *ifname, const struct in_addr *addr,
                         const struct in_addr *mask) {
  return PlNetworkSetIpv4AddrOsImpl(ifname, addr, mask);
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4Gateway(const char *ifname, const struct in_addr *addr) {
  return PlNetworkSetIpv4GatewayOsImpl(ifname, addr);
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4Dns(const char *ifname, const struct in_addr *addr) {
  return PlNetworkSetIpv4DnsOsImpl(ifname, addr);
}

// -----------------------------------------------------------------------------
int PlNetworkSetIpv4Method(const char *ifname, bool is_dhcp) {
  return PlNetworkSetIpv4MethodOsImpl(ifname, is_dhcp);
}

// -----------------------------------------------------------------------------
int PlNetworkGetIpv4Addr(const char *ifname, struct in_addr *addr,
                         struct in_addr *mask) {
  return PlNetworkGetIpv4AddrOsImpl(ifname, addr, mask);
}

// -----------------------------------------------------------------------------
int PlNetworkGetIpv4Gateway(const char *ifname, struct in_addr *addr) {
  return PlNetworkGetIpv4GatewayOsImpl(ifname, addr);
}

// -----------------------------------------------------------------------------
int PlNetworkGetIpv4Dns(const char *ifname, struct in_addr *addr) {
  return PlNetworkGetIpv4DnsOsImpl(ifname, addr);
}

// -----------------------------------------------------------------------------
int PlNetworkGetMacAddr(const char *ifname, uint8_t *mac) {
  return PlNetworkGetMacAddrOsImpl(ifname, mac);
}

// -----------------------------------------------------------------------------
void *PlNetworkDhcpcOpen(const char *ifname, void *mac, int maclen) {
  return PlNetworkDhcpcOpenOsImpl(ifname, mac, maclen);
}

// -----------------------------------------------------------------------------
int PlNetworkDhcpcRequest(void *handle, struct PlNetworkDhcpcState *state) {
  return PlNetworkDhcpcRequestOsImpl(handle, state);
}

// -----------------------------------------------------------------------------
int PlNetworkDhcpcRenew(void *handle, struct PlNetworkDhcpcState *state) {
  return PlNetworkDhcpcRenewOsImpl(handle, state);
}

// -----------------------------------------------------------------------------
int PlNetworkDhcpcRelease(void *handle, struct PlNetworkDhcpcState *state) {
  return PlNetworkDhcpcReleaseOsImpl(handle, state);
}

// -----------------------------------------------------------------------------
void PlNetworkDhcpcClose(void *handle) {
  return PlNetworkDhcpcCloseOsImpl(handle);
}

// -----------------------------------------------------------------------------
int PlNetworkResetDnsServer(void) {
  return PlNetworkResetDnsServerOsImpl();
}
