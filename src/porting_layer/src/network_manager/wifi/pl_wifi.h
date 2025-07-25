/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_WIFI_H_
#define PL_WIFI_H_

// Includes --------------------------------------------------------------------
#include <nuttx/config.h>

// Macros ----------------------------------------------------------------------
// Interface Name
#ifdef CONFIG_PL_WLAN0_IFNAME
# define  NETWORK_WLAN0_IFNAME            CONFIG_PL_WLAN0_IFNAME
#else  // CONFIG_PL_WLAN0_IFNAME
# define  NETWORK_WLAN0_IFNAME            "wlan0"
#endif  // CONFIG_PL_WLAN0_IFNAME

#ifdef CONFIG_PL_WLAN01_IFNAME
# define  NETWORK_WLAN1_IFNAME            CONFIG_PL_WLAN01_IFNAME
#else  // CONFIG_PL_WLAN01_IFNAME
# define  NETWORK_WLAN1_IFNAME            "wlan1"
#endif  // CONFIG_PL_WLAN01_IFNAME

// Cloud Connection
#ifdef CONFIG_PL_WLAN0_CLOUD
# define  NETWORK_WLAN0_CLOUD             true
#else  // CONFIG_PL_WLAN0_CLOUD
# define  NETWORK_WLAN0_CLOUD             false
#endif  // CONFIG_PL_WLAN0_CLOUD

#ifdef CONFIG_PL_WLAN1_CLOUD
# define  NETWORK_WLAN1_CLOUD             true
#else  // CONFIG_PL_WLAN1_CLOUD
# define  NETWORK_WLAN1_CLOUD             false
#endif  // CONFIG_PL_WLAN1_CLOUD

// Local Connection
#ifdef CONFIG_PL_WLAN0_LOCAL
# define  NETWORK_WLAN0_LOCAL             true
#else  // CONFIG_PL_WLAN0_LOCAL
# define  NETWORK_WLAN0_LOCAL             false
#endif  // CONFIG_PL_WLAN0_LOCAL

#ifdef CONFIG_PL_WLAN1_LOCAL
# define  NETWORK_WLAN1_LOCAL             true
#else  // CONFIG_PL_WLAN1_LOCAL
# define  NETWORK_WLAN1_LOCAL             false
#endif  // CONFIG_PL_WLAN1_LOCAL

// devive
#ifdef CONFIG_PL_WLAN0_HAVE_DEVICE
# define WLAN0_DEVICE_ENABLE              true
#else  // CONFIG_PL_WLAN0_HAVE_DEVICE
# define WLAN0_DEVICE_ENABLE              false
#endif  // CONFIG_PL_WLAN0_HAVE_DEVICE

#ifdef CONFIG_PL_WLAN0_DEVNAME
# define WLAN0_DEVICE_NAME                CONFIG_PL_WLAN0_DEVNAME
#else  // CONFIG_PL_WLAN0_DEVNAME
# define WLAN0_DEVICE_NAME                "\0"
#endif  // CONFIG_PL_WLAN0_DEVNAME

#ifdef CONFIG_PL_WLAN1_HAVE_DEVICE
# define WLAN1_DEVICE_ENABLE              true
#else  // CONFIG_PL_WLAN1_HAVE_DEVICE
# define WLAN1_DEVICE_ENABLE              false
#endif  // CONFIG_PL_WLAN1_HAVE_DEVICE

#ifdef CONFIG_PL_WLAN1_DEVNAME
# define WLAN1_DEVICE_NAME                CONFIG_PL_WLAN1_DEVNAME
#else  // CONFIG_PL_WLAN1_DEVNAME
# define WLAN1_DEVICE_NAME                "\0"
#endif  // CONFIG_PL_WLAN1_DEVNAME

// Functions -------------------------------------------------------------------
// pl_wifi.c
PlErrCode PlWifiInitialize(struct network_info *net_info,
                           uint32_t thread_priority);
PlErrCode PlWifiFinalize(struct network_info *net_info);
PlErrCode PlWifiSetConfig(struct network_info *net_info,
                          const PlNetworkConfig *config);
PlErrCode PlWifiGetConfig(struct network_info *net_info,
                          PlNetworkConfig *config);
PlErrCode PlWifiGetStatus(struct network_info *net_info,
                          PlNetworkStatus *status);
PlErrCode PlWifiRegisterEventHandler(struct network_info *net_info);
PlErrCode PlWifiUnregisterEventHandler(struct network_info *net_info);
PlErrCode PlWifiStart(struct network_info *net_info);
PlErrCode PlWifiStop(struct network_info *net_info);

#endif  // PL_WIFI_H_
