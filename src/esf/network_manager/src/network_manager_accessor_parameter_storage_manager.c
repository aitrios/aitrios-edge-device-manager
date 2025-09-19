/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// self header.
#include "network_manager/network_manager_accessor_parameter_storage_manager.h"

// system header
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// other header
#include "network_manager.h"
#include "network_manager/network_manager_log.h"
#include "network_manager/network_manager_resource.h"
#include "parameter_storage_manager.h"

// Structure defines the mapping of members between the internal processing
// structure and the externally exposed structure.
typedef struct EsfNetworkManagerParameterMemberMap {
  size_t offset;
  size_t shift;
  EsfNetworkManagerConvertType convert;
} EsfNetworkManagerParameterMemberMap;

// """Determine if enrollment mask "normal_mode.dev_ip.ip" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip.ip" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.dev_ip.ip" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIp(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip.subnet_mask" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip.subnet_mask" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "normal_mode.dev_ip.subnet_mask" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalSubnetMask(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip.gateway" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip.gateway" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.dev_ip.gateway" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalGateway(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip.dns" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip.dns" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.dev_ip.dns" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalDns(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip_v6.ip" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip_v6.ip" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.dev_ip_v6.ip" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Ip(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip_v6.subnet_mask" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip_v6.subnet_mask" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "normal_mode.dev_ip_v6.subnet_mask" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6SubnetMask(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip_v6.gateway" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip_v6.gateway" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.dev_ip_v6.gateway"
//     is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Gateway(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.dev_ip_v6.dns" is valid.

// Returns true if the enrollment mask "normal_mode.dev_ip_v6.dns" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.dev_ip_v6.dns" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Dns(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.wifi_sta.ssid" is valid.

// Returns true if the enrollment mask "normal_mode.wifi_sta.ssid" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.wifi_sta.ssid" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiSsid(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.wifi_sta.password" is valid.

// Returns true if the enrollment mask "normal_mode.wifi_sta.password" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.wifi_sta.password"
//     is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiPassword(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.wifi_sta.encryption" is valid.

// Returns true if the enrollment mask "normal_mode.wifi_sta.encryption" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "normal_mode.wifi_sta.encryption" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiEncryption(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.ip_method" is valid.

// Returns true if the enrollment mask "normal_mode.ip_method" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.ip_method" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpMethod(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "normal_mode.netif_kind" is valid.

// Returns true if the enrollment mask "normal_mode.netif_kind" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "normal_mode.netif_kind" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalNetifKind(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.dev_ip.ip" is valid.

// Returns true if the enrollment mask "accesspoint_mode.dev_ip.ip" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "accesspoint_mode.dev_ip.ip" is
//     valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointIp(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.dev_ip.subnet_mask" is
// valid.

// Returns true if the enrollment mask "accesspoint_mode.dev_ip.subnet_mask" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "accesspoint_mode.dev_ip.subnet_mask" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointSubnetMask(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.dev_ip.gateway" is valid.

// Returns true if the enrollment mask "accesspoint_mode.dev_ip.gateway" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "accesspoint_mode.dev_ip.gateway" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointGateway(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.dev_ip.dns" is valid.

// Returns true if the enrollment mask "accesspoint_mode.dev_ip.dns" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "accesspoint_mode.dev_ip.dns"
//     is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointDns(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.wifi_ap.ssid" is valid.

// Returns true if the enrollment mask "accesspoint_mode.wifi_ap.ssid" is valid,
// otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "accesspoint_mode.wifi_ap.ssid"
//     is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiSsid(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.wifi_ap.password" is valid.

// Returns true if the enrollment mask "accesspoint_mode.wifi_ap.password" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "accesspoint_mode.wifi_ap.password" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiPassword(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.wifi_ap.encryption" is
// valid.

// Returns true if the enrollment mask "accesspoint_mode.wifi_ap.encryption" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "accesspoint_mode.wifi_ap.encryption" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiEncryption(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "accesspoint_mode.wifi_ap.channel" is valid.

// Returns true if the enrollment mask "accesspoint_mode.wifi_ap.channel" is
// valid, otherwise returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask
//     "accesspoint_mode.wifi_ap.channel" is valid, otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiChannel(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "proxy.url" is valid.

// Returns true if the enrollment mask "proxy.url" is valid, otherwise returns
// false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "proxy.url" is valid, otherwise
//     returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyUrl(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "proxy.port" is valid.

// Returns true if the enrollment mask "proxy.port" is valid, otherwise returns
// false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "proxy.port" is valid,
//     otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyPort(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "proxy.username" is valid.

// Returns true if the enrollment mask "proxy.username" is valid, otherwise
// returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "proxy.username" is valid,
//     otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyUsername(
    EsfParameterStorageManagerMask mask);

// """Determine if enrollment mask "proxy.password" is valid.

// Returns true if the enrollment mask "proxy.password" is valid, otherwise
// returns false.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.

// Returns:
//     bool: Returns true if the enrollment mask "proxy.password" is valid,
//     otherwise returns false.

// Note:
// """
static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyPassword(
    EsfParameterStorageManagerMask mask);

// """Copies data from an external structure to an internal structure.

// Copies data from an external structure to an internal structure.
// Only data with a valid mask is copied.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.
//     [IN] src (const EsfNetworkManagerParameter *):
//       external structure.
//     [OUT] dst (EsfNetworkManagerParameterInternal *):
//       internal structure.

// Note:
// """
static void
EsfNetworkManagerParameterStorageManagerAccessorCopyToInternalStructure(
    EsfParameterStorageManagerMask mask, const EsfNetworkManagerParameter *src,
    EsfNetworkManagerParameterInternal *dst);

// """Copies data from an internal structure to an external structure.

// Copies data from an internal structure to an external structure.
// Only data with a valid mask is copied.

// Args:
//     [IN] mask (EsfParameterStorageManagerMask):
//       EsfParameterStorageManagerMask.
//     [IN] src (const EsfNetworkManagerParameterInternal *):
//       internal structure.
//     [OUT] dst (EsfNetworkManagerParameter *):
//       external structure.

// Note:
// """
static void
EsfNetworkManagerParameterStorageManagerAccessorCopyToExternalStructure(
    EsfParameterStorageManagerMask mask,
    const EsfNetworkManagerParameterInternal *src,
    EsfNetworkManagerParameter *dst);

// """Show internal access parameter.

// Show internal access parameter.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure.
//     parameter (const EsfNetworkManagerParameterInternal *):
//       A structure containing the parameters to be checked.

// Note:
// """
static void
EsfNetworkManagerParameterStorageManagerAccessorShowInternalParameter(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameterInternal *parameter);

// ParameterStorageManagerAccessMemberInfo.
static const EsfParameterStorageManagerMemberInfo kEsfNetworkManagerParameterStorageManagerAccessorMemberInfo[] = {
    {
        .id = kEsfParameterStorageManagerItemIPAddress,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip.ip),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, param.normal_mode.dev_ip.ip),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIp,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemSubnetMask,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip.subnet_mask),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.dev_ip.subnet_mask),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalSubnetMask,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemGateway,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip.gateway),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.dev_ip.gateway),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalGateway,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemDNS,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip.dns),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, param.normal_mode.dev_ip.dns),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalDns,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemIPv6IPAddress,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip_v6.ip),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, param.normal_mode.dev_ip_v6.ip),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Ip,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemIPv6SubnetMask,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip_v6.subnet_mask),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.dev_ip_v6.subnet_mask),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6SubnetMask,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemIPv6Gateway,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip_v6.gateway),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.dev_ip_v6.gateway),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Gateway,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemIPv6DNS,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.dev_ip_v6.dns),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.dev_ip_v6.dns),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Dns,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiSSID,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.wifi_sta.ssid),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.wifi_sta.ssid),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiSsid,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiPassword,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.normal_mode.wifi_sta.password),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.normal_mode.wifi_sta.password),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiPassword,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiEncryption,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset =
            offsetof(EsfNetworkManagerParameterInternal, wifi_sta_encryption),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfNetworkManagerInt32Parameter, data),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiEncryption,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemIPMethod,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset = offsetof(EsfNetworkManagerParameterInternal, ip_method),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfNetworkManagerInt32Parameter, data),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpMethod,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemNetIfKind,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset = offsetof(EsfNetworkManagerParameterInternal, netif_kind),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfNetworkManagerInt32Parameter, data),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalNetifKind,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApIPAddress,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.accesspoint_mode.dev_ip.ip),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.accesspoint_mode.dev_ip.ip),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointIp,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApSubnetMask,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.accesspoint_mode.dev_ip.subnet_mask),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.accesspoint_mode.dev_ip.subnet_mask),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointSubnetMask,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApGateway,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.accesspoint_mode.dev_ip.gateway),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.accesspoint_mode.dev_ip.gateway),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointGateway,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApDNS,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.accesspoint_mode.dev_ip.dns),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.accesspoint_mode.dev_ip.dns),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointDns,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApSSID,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.accesspoint_mode.wifi_ap.ssid),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.accesspoint_mode.wifi_ap.ssid),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiSsid,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApPassword,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal,
                           param.accesspoint_mode.wifi_ap.password),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal,
            param.accesspoint_mode.wifi_ap.password),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiPassword,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApEncryption,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset =
            offsetof(EsfNetworkManagerParameterInternal, wifi_ap_encryption),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfNetworkManagerInt32Parameter, data),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiEncryption,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemWiFiApChannel,
        .type = kEsfParameterStorageManagerItemTypeRaw,
        .offset = offsetof(EsfNetworkManagerParameterInternal, wifi_ap_channel),
        .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
            EsfNetworkManagerInt32Parameter, data),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiChannel,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemProxyURL,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal, param.proxy.url),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, param.proxy.url),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyUrl,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemProxyPort,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset = offsetof(EsfNetworkManagerParameterInternal, proxy_port),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, proxy_port),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyPort,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemProxyUserName,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset =
            offsetof(EsfNetworkManagerParameterInternal, param.proxy.username),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, param.proxy.username),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyUsername,
        .custom = NULL,
    },
    {
        .id = kEsfParameterStorageManagerItemProxyPassword,
        .type = kEsfParameterStorageManagerItemTypeString,
        .offset =
            offsetof(EsfNetworkManagerParameterInternal, param.proxy.password),
        .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
            EsfNetworkManagerParameterInternal, param.proxy.password),
        .enabled =
            EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyPassword,
        .custom = NULL,
    },
};

// ParameterStorageManager access structure information.
static const EsfParameterStorageManagerStructInfo
    kEsfNetworkManagerParameterStorageManagerAccessorStructInfo = {
        .items_num =
            sizeof(
                kEsfNetworkManagerParameterStorageManagerAccessorMemberInfo) /
            sizeof(
                kEsfNetworkManagerParameterStorageManagerAccessorMemberInfo[0]),
        .items = kEsfNetworkManagerParameterStorageManagerAccessorMemberInfo,
};

//
static const EsfNetworkManagerParameterMemberMap kExternalStructMapping[] = {
    {
        .offset = offsetof(EsfNetworkManagerParameter, normal_mode.dev_ip.ip),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           normal_mode.dev_ip.subnet_mask),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, normal_mode.dev_ip.gateway),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, normal_mode.dev_ip.dns),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, normal_mode.dev_ip_v6.ip),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           normal_mode.dev_ip_v6.subnet_mask),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, normal_mode.dev_ip_v6.gateway),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, normal_mode.dev_ip_v6.dns),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, normal_mode.wifi_sta.ssid),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, normal_mode.wifi_sta.password),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           normal_mode.wifi_sta.encryption),
        .shift = offsetof(EsfNetworkManagerInt32Parameter, data),
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, normal_mode.ip_method),
        .shift = offsetof(EsfNetworkManagerInt32Parameter, data),
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, normal_mode.netif_kind),
        .shift = offsetof(EsfNetworkManagerInt32Parameter, data),
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, accesspoint_mode.dev_ip.ip),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           accesspoint_mode.dev_ip.subnet_mask),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           accesspoint_mode.dev_ip.gateway),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, accesspoint_mode.dev_ip.dns),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset =
            offsetof(EsfNetworkManagerParameter, accesspoint_mode.wifi_ap.ssid),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           accesspoint_mode.wifi_ap.password),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           accesspoint_mode.wifi_ap.encryption),
        .shift = offsetof(EsfNetworkManagerInt32Parameter, data),
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter,
                           accesspoint_mode.wifi_ap.channel),
        .shift = offsetof(EsfNetworkManagerInt32Parameter, data),
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, proxy.url),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, proxy.port),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeIntToString,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, proxy.username),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
    {
        .offset = offsetof(EsfNetworkManagerParameter, proxy.password),
        .shift = 0,
        .convert = kEsfNetworkManagerConvertTypeNoConvert,
    },
};

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIp(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip.ip, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalSubnetMask(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip.subnet_mask, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalGateway(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip.gateway, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalDns(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip.dns, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Ip(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip_v6.ip, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6SubnetMask(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip_v6.subnet_mask, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Gateway(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip_v6.gateway, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpv6Dns(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.dev_ip_v6.dns, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiSsid(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.wifi_sta.ssid, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiPassword(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.wifi_sta.password, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalWifiEncryption(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.wifi_sta.encryption, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpMethod(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.ip_method, mask);
}

// """Determine if enrollment mask "normal_mode.netif_kind" is valid.

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalNetifKind(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, normal_mode.netif_kind, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointIp(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.dev_ip.ip, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointSubnetMask(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.dev_ip.subnet_mask,
      mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointGateway(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.dev_ip.gateway, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointDns(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.dev_ip.dns, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiSsid(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.wifi_ap.ssid, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiPassword(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.wifi_ap.password, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiEncryption(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.wifi_ap.encryption,
      mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkAccessPointWifiChannel(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, accesspoint_mode.wifi_ap.channel, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyUrl(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, proxy.url, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyPort(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, proxy.port, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyUsername(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, proxy.username, mask);
}

static bool
EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkProxyPassword(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfNetworkManagerParameterMask, proxy.password, mask);
}

static void
EsfNetworkManagerParameterStorageManagerAccessorCopyToInternalStructure(
    EsfParameterStorageManagerMask mask, const EsfNetworkManagerParameter *src,
    EsfNetworkManagerParameterInternal *dst) {
  ESF_NETWORK_MANAGER_TRACE("START mask=0x%" PRIxPTR " src=%p dst=%p", mask,
                            src, dst);
  uintptr_t src_addr = (uintptr_t)src;
  uintptr_t dst_addr = (uintptr_t)dst;

  for (size_t i = 0;
       i <
       kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items_num;
       ++i) {
    if (!kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items[i]
             .enabled(mask)) {
      continue;
    }
    const size_t offset =
        kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items[i]
            .offset;
    const size_t size =
        kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items[i]
            .size;
    switch (kExternalStructMapping[i].convert) {
      case kEsfNetworkManagerConvertTypeIntToString:
        snprintf((char *)(dst_addr + offset + kExternalStructMapping[i].shift),
                 size, "%d",
                 *((int32_t *)(src_addr + kExternalStructMapping[i].offset)));
        ESF_NETWORK_MANAGER_DBG(
            "port conv to internal dst:%s src:%d",
            (char *)(dst_addr + offset + kExternalStructMapping[i].shift),
            *((int32_t *)(src_addr + kExternalStructMapping[i].offset)));
        break;
      case kEsfNetworkManagerConvertTypeNoConvert:
      default:
        memcpy((void *)(dst_addr + offset + kExternalStructMapping[i].shift),
               (const void *)(src_addr + kExternalStructMapping[i].offset),
               size - kExternalStructMapping[i].shift);
        break;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

static void
EsfNetworkManagerParameterStorageManagerAccessorCopyToExternalStructure(
    EsfParameterStorageManagerMask mask,
    const EsfNetworkManagerParameterInternal *src,
    EsfNetworkManagerParameter *dst) {
  ESF_NETWORK_MANAGER_TRACE("START mask=0x%" PRIxPTR " src=%p dst=%p", mask,
                            src, dst);
  uintptr_t src_addr = (uintptr_t)src;
  uintptr_t dst_addr = (uintptr_t)dst;

  for (size_t i = 0;
       i <
       kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items_num;
       ++i) {
    if (!kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items[i]
             .enabled(mask)) {
      continue;
    }
    const size_t offset =
        kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items[i]
            .offset;
    const size_t size =
        kEsfNetworkManagerParameterStorageManagerAccessorStructInfo.items[i]
            .size;
    switch (kExternalStructMapping[i].convert) {
      case kEsfNetworkManagerConvertTypeIntToString: {
        char *p_end;
        const char *p_src_char =
            (const char *)(src_addr + offset + kExternalStructMapping[i].shift);
        int32_t int_tmp = (int32_t)strtol(p_src_char, &p_end, 10);
        if (*p_end != '\0' || *p_src_char == '\0') {
          ESF_NETWORK_MANAGER_INFO("strtol convert failure %s", p_src_char);
          int_tmp = 0;
        }
        memcpy((void *)(dst_addr + kExternalStructMapping[i].offset),
               (const void *)(&int_tmp), sizeof(int_tmp));
        ESF_NETWORK_MANAGER_DBG(
            "port conv to external dst:%d dst2:%d src:%s", int_tmp,
            *((int32_t *)(dst_addr + kExternalStructMapping[i].offset)),
            p_src_char);
        break;
      }
      case kEsfNetworkManagerConvertTypeNoConvert:
      default:
        memcpy(
            (void *)(dst_addr + kExternalStructMapping[i].offset),
            (const void *)(src_addr + offset + kExternalStructMapping[i].shift),
            size - kExternalStructMapping[i].shift);
        break;
    }
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

static void
EsfNetworkManagerParameterStorageManagerAccessorShowInternalParameter(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameterInternal *parameter) {
  ESF_NETWORK_MANAGER_TRACE("START");
  ESF_NETWORK_MANAGER_DBG("  NORMAL MODE");
  if (mask->normal_mode.dev_ip.ip) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.ip         :%s",
                            parameter->param.normal_mode.dev_ip.ip);
  }
  if (mask->normal_mode.dev_ip.subnet_mask) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.subnet_mask:%s",
                            parameter->param.normal_mode.dev_ip.subnet_mask);
  }
  if (mask->normal_mode.dev_ip.gateway) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.gateway    :%s",
                            parameter->param.normal_mode.dev_ip.gateway);
  }
  if (mask->normal_mode.dev_ip.dns) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip.dns        :%s",
                            parameter->param.normal_mode.dev_ip.dns);
  }
  if (mask->normal_mode.dev_ip_v6.ip) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.ip      :%s",
                            parameter->param.normal_mode.dev_ip_v6.ip);
  }
  if (mask->normal_mode.dev_ip_v6.subnet_mask) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.subnet_mask:%s",
                            parameter->param.normal_mode.dev_ip_v6.subnet_mask);
  }
  if (mask->normal_mode.dev_ip_v6.gateway) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.gateway :%s",
                            parameter->param.normal_mode.dev_ip_v6.gateway);
  }
  if (mask->normal_mode.dev_ip_v6.dns) {
    ESF_NETWORK_MANAGER_DBG("    dev_ip_v6.dns     :%s",
                            parameter->param.normal_mode.dev_ip_v6.dns);
  }
  if (mask->normal_mode.ip_method) {
    ESF_NETWORK_MANAGER_DBG("    ip_method         :%d",
                            parameter->ip_method.data);
  }
  if (mask->normal_mode.netif_kind) {
    ESF_NETWORK_MANAGER_DBG("    netif_kind        :%d",
                            parameter->netif_kind.data);
  }
  if (mask->normal_mode.wifi_sta.ssid) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.ssid     :%s",
                            parameter->param.normal_mode.wifi_sta.ssid);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.ssid     :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  if (mask->normal_mode.wifi_sta.password) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.password :%s",
                            parameter->param.normal_mode.wifi_sta.password);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.password :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  if (mask->normal_mode.wifi_sta.encryption) {
    ESF_NETWORK_MANAGER_DBG("    wifi_sta.encryption:%d",
                            parameter->wifi_sta_encryption.data);
  }
  ESF_NETWORK_MANAGER_DBG("  PROXY");
  if (mask->proxy.url) {
    ESF_NETWORK_MANAGER_DBG("    proxy.url         :%s",
                            parameter->param.proxy.url);
  }
  if (mask->proxy.port) {
    ESF_NETWORK_MANAGER_DBG("    proxy.port        :%s", parameter->proxy_port);
  }
  if (mask->proxy.username) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.username    :%s",
                            parameter->param.proxy.username);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.username    :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  if (mask->proxy.password) {
#ifdef ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.password    :%s",
                            parameter->param.proxy.password);
#else   // ENABLE_SECURITY_SECRET_LOG
    ESF_NETWORK_MANAGER_DBG("    proxy.password    :Secret - Not display");
#endif  // ENABLE_SECURITY_SECRET_LOG
  }
  ESF_NETWORK_MANAGER_TRACE("END");
}

EsfNetworkManagerResult
EsfNetworkManagerParameterStorageManagerAccessorSaveNetwork(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *data) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p data=%p", mask, data);
  if (mask == NULL || data == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask=%p, data=%p)", mask, data);
    return kEsfNetworkManagerResultInvalidParameter;
  }
  EsfNetworkManagerParameterInternal *internal_data = NULL;
  EsfNetworkManagerResult ret =
      EsfNetworkManagerResourceGetParameterInternalWork(&internal_data);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("GetParameterInternalWork error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultResourceExhausted;
  }
  memset(internal_data, 0, sizeof(*internal_data));
  EsfNetworkManagerParameterStorageManagerAccessorCopyToInternalStructure(
      (EsfParameterStorageManagerMask)mask, data, internal_data);

  ESF_NETWORK_MANAGER_DBG("Save internal parameter.");
  EsfNetworkManagerParameterStorageManagerAccessorShowInternalParameter(
      mask, internal_data);
  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    EsfParameterStorageManagerStatus storage_result =
        kEsfParameterStorageManagerStatusOk;
    EsfParameterStorageManagerHandle handle =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
    storage_result = EsfParameterStorageManagerOpen(&handle);
    if (storage_result != kEsfParameterStorageManagerStatusOk) {
      ESF_NETWORK_MANAGER_ERR("ParameterStorageManagerOpen error.(result=%d)",
                              storage_result);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
      return_result = kEsfNetworkManagerResultExternalError;
      break;
    }

    storage_result = EsfParameterStorageManagerSave(
        handle, (EsfParameterStorageManagerMask)mask,
        (EsfParameterStorageManagerData)internal_data,
        &kEsfNetworkManagerParameterStorageManagerAccessorStructInfo, NULL);
    if (storage_result != kEsfParameterStorageManagerStatusOk) {
      ESF_NETWORK_MANAGER_ERR("ParameterStorageManagerSave error.(result=%d)",
                              storage_result);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
      return_result = kEsfNetworkManagerResultExternalError;
    }

    storage_result = EsfParameterStorageManagerClose(handle);
    if (storage_result != kEsfParameterStorageManagerStatusOk) {
      ESF_NETWORK_MANAGER_ERR("ParameterStorageManagerClose error.(result=%d)",
                              storage_result);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
      return_result = kEsfNetworkManagerResultExternalError;
    }
  } while (0);

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}

EsfNetworkManagerResult
EsfNetworkManagerParameterStorageManagerAccessorLoadNetwork(
    const EsfNetworkManagerParameterMask *mask,
    EsfNetworkManagerParameter *data) {
  ESF_NETWORK_MANAGER_TRACE("START mask=%p data=%p", mask, data);
  if (mask == NULL || data == NULL) {
    ESF_NETWORK_MANAGER_ERR("parameter error.(mask=%p, data=%p)", mask, data);
    return kEsfNetworkManagerResultInvalidParameter;
  }

  EsfNetworkManagerParameterInternal *internal_data = NULL;
  EsfNetworkManagerResult ret =
      EsfNetworkManagerResourceGetParameterInternalWork(&internal_data);
  if (ret != kEsfNetworkManagerResultSuccess) {
    ESF_NETWORK_MANAGER_ERR("GetParameterInternalWork error.");
    ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_INTERNAL_ERROR);
    return kEsfNetworkManagerResultResourceExhausted;
  }
  memset(internal_data, 0, sizeof(*internal_data));

  EsfNetworkManagerResult return_result = kEsfNetworkManagerResultSuccess;
  do {
    EsfParameterStorageManagerStatus storage_result =
        kEsfParameterStorageManagerStatusOk;
    EsfParameterStorageManagerHandle handle =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
    storage_result = EsfParameterStorageManagerOpen(&handle);
    if (storage_result != kEsfParameterStorageManagerStatusOk) {
      ESF_NETWORK_MANAGER_ERR("ParameterStorageManagerOpen error.(result=%d)",
                              storage_result);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
      return_result = kEsfNetworkManagerResultExternalError;
      break;
    }

    storage_result = EsfParameterStorageManagerLoad(
        handle, (EsfParameterStorageManagerMask)mask,
        (EsfParameterStorageManagerData)internal_data,
        &kEsfNetworkManagerParameterStorageManagerAccessorStructInfo, NULL);
    if (storage_result != kEsfParameterStorageManagerStatusOk) {
      ESF_NETWORK_MANAGER_ERR("ParameterStorageManagerLoad error.(result=%d)",
                              storage_result);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
      return_result = kEsfNetworkManagerResultExternalError;
    } else {
      ESF_NETWORK_MANAGER_DBG("Load internal parameter.");
      EsfNetworkManagerParameterStorageManagerAccessorShowInternalParameter(
          mask, internal_data);
    }

    storage_result = EsfParameterStorageManagerClose(handle);
    if (storage_result != kEsfParameterStorageManagerStatusOk) {
      ESF_NETWORK_MANAGER_ERR("ParameterStorageManagerClose error.(result=%d)",
                              storage_result);
      ESF_NETWORK_MANAGER_ELOG_ERR(ESF_NETWORK_MANAGER_ELOG_FLASH_FAILURE);
      return_result = kEsfNetworkManagerResultExternalError;
    }

    EsfNetworkManagerParameterStorageManagerAccessorCopyToExternalStructure(
        (EsfParameterStorageManagerMask)mask, internal_data, data);

    // Setup initial values
    if (EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalIpMethod(
            (EsfParameterStorageManagerMask)mask) &&
        internal_data->ip_method.size == 0) {
      data->normal_mode.ip_method = 0;  // DHCP
    }
    // NetifKind
    // This process is exclusive to T5.
    // For T3, PlStorage responds with fixed values,
    // so the size does not become 0.
    if (EsfNetworkManagerParameterStorageManagerAccessorMaskEnabledNetworkNormalNetifKind(
            (EsfParameterStorageManagerMask)mask) &&
        internal_data->netif_kind.size == 0) {
#ifdef CONFIG_EXTERNAL_TARGET_T5
      data->normal_mode.netif_kind = 1;  // Ether
#else
      data->normal_mode.netif_kind = 0;
#endif
    }
  } while (0);

  ESF_NETWORK_MANAGER_TRACE("END");
  return return_result;
}
