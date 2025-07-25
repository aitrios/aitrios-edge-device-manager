/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_INTERNAL_H_
#define ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_INTERNAL_H_

// Network interface type definition.
// To be used with netif_kind in EsfNetworkManagerNormalMode,
// the value must match the external definition.
typedef enum EsfNetworkManagerNetifKind {
  // WiFi
  kEsfNetworkManagerNetifKindWiFi = 0,

  // Ether
  kEsfNetworkManagerNetifKindEther,
} EsfNetworkManagerNetifKind;

// Select the IP address method definition.
// To be used with ip_method in EsfNetworkManagerNormalMode,
// the value must match the external definition.
typedef enum EsfNetworkManagerIpMethod {
  // DHCP
  kEsfNetworkManagerIpMethodDhcp = 0,

  // Static
  kEsfNetworkManagerIpMethodStatic,
} EsfNetworkManagerIpMethod;
#endif  // ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_INTERNAL_H_
