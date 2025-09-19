/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_WIFI_AP_H_
#define PL_WIFI_AP_H_

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_network.h"

// Public functions ------------------------------------------------------------
#ifdef CONFIG_PL_NETWORK_WIFI_AP
PlErrCode PlWifiApSetConfig(const char *if_name, const PlNetworkConfig *config);
PlErrCode PlWifiApGetConfig(const char *if_name, PlNetworkConfig *config);
PlErrCode PlWifiApGetStatus(const char *if_name, PlNetworkStatus *status);
PlErrCode PlWifiApRegisterEventHandler(const char *if_name);
PlErrCode PlWifiApUnregisterEventHandler(const char *if_name);
PlErrCode PlWifiApStart(const char *if_name);
PlErrCode PlWifiApStop(const char *if_name);
#else
#define PlWifiApSetConfig(if_name, config) (kPlErrNoSupported)
#define PlWifiApGetConfig(if_name, config) (kPlErrNoSupported)
#define PlWifiApGetStatus(if_name, status) (kPlErrNoSupported)
#define PlWifiApRegisterEventHandler(if_name) (kPlErrNoSupported)
#define PlWifiApUnregisterEventHandler(if_name) (kPlErrNoSupported)
#define PlWifiApStart(if_name) (kPlErrNoSupported)
#define PlWifiApStop(if_name) (kPlErrNoSupported)
#endif
#endif  // PL_WIFI_AP_H_
