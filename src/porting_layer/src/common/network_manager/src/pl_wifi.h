/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_WIFI_H_
#define PL_WIFI_H_

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"

// Public functions ------------------------------------------------------------
#ifdef CONFIG_PL_NETWORK_HAVE_WIFI
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
#else
#define PlWifiInitialize(net_info, thread_priority) (kPlErrNoSupported)
#define PlWifiFinalize(net_info) (kPlErrNoSupported)
#define PlWifiSetConfig(net_info, config) (kPlErrNoSupported)
#define PlWifiGetConfig(net_info, config) (kPlErrNoSupported)
#define PlWifiGetStatus(net_info, status) (kPlErrNoSupported)
#define PlWifiRegisterEventHandler(net_info) (kPlErrNoSupported)
#define PlWifiUnregisterEventHandler(net_info) (kPlErrNoSupported)
#define PlWifiStart(net_info) (kPlErrNoSupported)
#define PlWifiStop(net_info) (kPlErrNoSupported);
#endif

#endif  // PL_WIFI_H_
