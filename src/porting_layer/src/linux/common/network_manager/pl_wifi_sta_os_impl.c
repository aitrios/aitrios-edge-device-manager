/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_wifi_sta_os_impl.h"

#include "pl.h"
#include "pl_network.h"

// Public functions ------------------------------------------------------------
PlErrCode PlWifiStaSetConfigOsImpl(const char *if_name,
                                   const PlNetworkConfig *config) {
  // Do nothing
  (void)if_name;
  (void)config;
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaGetConfigOsImpl(const char *if_name,
                                   PlNetworkConfig *config) {
  // Do nothing
  (void)if_name;
  (void)config;
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaGetStatusOsImpl(const char *if_name,
                                   PlNetworkStatus *status) {
  // Do nothing
  (void)if_name;
  (void)status;
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaRegisterEventHandlerOsImpl(const char *if_name) {
  // Do nothing
  (void)if_name;
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaUnregisterEventHandlerOsImpl(const char *if_name) {
  // Do nothing
  (void)if_name;
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaStartOsImpl(const char *if_name) {
  // Do nothing
  (void)if_name;
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlWifiStaStopOsImpl(const char *if_name) {
  // Do nothing
  (void)if_name;
  return kPlErrNoSupported;
}
