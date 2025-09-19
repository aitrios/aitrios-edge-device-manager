/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_WIFI_STA_OS_IMPL_H_
#define PL_WIFI_STA_OS_IMPL_H_

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_network.h"

// Public functions ------------------------------------------------------------
PlErrCode PlWifiStaSetConfigOsImpl(const char *if_name,
                                   const PlNetworkConfig *config);
PlErrCode PlWifiStaGetConfigOsImpl(const char *if_name,
                                   PlNetworkConfig *config);
PlErrCode PlWifiStaGetStatusOsImpl(const char *if_name,
                                   PlNetworkStatus *status);
PlErrCode PlWifiStaRegisterEventHandlerOsImpl(const char *if_name);
PlErrCode PlWifiStaUnregisterEventHandlerOsImpl(const char *if_name);
PlErrCode PlWifiStaStartOsImpl(const char *if_name);
PlErrCode PlWifiStaStopOsImpl(const char *if_name);

#endif  // PL_WIFI_STA_OS_IMPL_H_
