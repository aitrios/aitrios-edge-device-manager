/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_wifi_sta.h"

#include "pl.h"
#include "pl_network.h"
#include "pl_wifi_sta_os_impl.h"

// Public functions ------------------------------------------------------------
PlErrCode PlWifiStaSetConfig(const char *if_name,
                             const PlNetworkConfig *config) {
  return PlWifiStaSetConfigOsImpl(if_name, config);
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaGetConfig(const char *if_name, PlNetworkConfig *config) {
  return PlWifiStaGetConfigOsImpl(if_name, config);
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaGetStatus(const char *if_name, PlNetworkStatus *status) {
  return PlWifiStaGetStatusOsImpl(if_name, status);
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaRegisterEventHandler(const char *if_name) {
  return PlWifiStaRegisterEventHandlerOsImpl(if_name);
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaUnregisterEventHandler(const char *if_name) {
  return PlWifiStaUnregisterEventHandlerOsImpl(if_name);
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaStart(const char *if_name) {
  return PlWifiStaStartOsImpl(if_name);
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaStop(const char *if_name) {
  return PlWifiStaStopOsImpl(if_name);
}
