/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_WIFI_STA_H_
#define PL_WIFI_STA_H_

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_network.h"

// Public functions ------------------------------------------------------------
#ifdef CONFIG_PL_NETWORK_WIFI_STATION
PlErrCode PlWifiStaSetConfig(const char *if_name,
                             const PlNetworkConfig *config);
PlErrCode PlWifiStaGetConfig(const char *if_name, PlNetworkConfig *config);
PlErrCode PlWifiStaGetStatus(const char *if_name, PlNetworkStatus *status);
PlErrCode PlWifiStaRegisterEventHandler(const char *if_name);
PlErrCode PlWifiStaUnregisterEventHandler(const char *if_name);
PlErrCode PlWifiStaStart(const char *if_name);
PlErrCode PlWifiStaStop(const char *if_name);
#else
#define PlWifiStaSetConfig(if_name, config) (kPlErrNoSupported)
#define PlWifiStaGetConfig(if_name, config) (kPlErrNoSupported)
#define PlWifiStaGetStatus(if_name, status) (kPlErrNoSupported)
#define PlWifiStaRegisterEventHandler(if_name) (kPlErrNoSupported)
#define PlWifiStaUnregisterEventHandler(if_name) (kPlErrNoSupported)
#define PlWifiStaStart(if_name) (kPlErrNoSupported)
#define PlWifiStaStop(if_name) (kPlErrNoSupported)
#endif
#endif  // PL_WIFI_STA_H_
