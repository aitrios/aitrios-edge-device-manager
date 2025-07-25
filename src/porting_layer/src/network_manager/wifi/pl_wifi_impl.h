/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_WIFI_IMPL_H_
#define PL_WIFI_IMPL_H_

// Includes --------------------------------------------------------------------


// Macros ----------------------------------------------------------------------


// Functions -------------------------------------------------------------------
// pl_wifi_sta_impl.c
#ifdef CONFIG_PL_NETWORK_WIFI_STATION
PlErrCode PlWifiStaSetConfigImpl(const char *if_name,
                                 const PlNetworkConfig *config);
PlErrCode PlWifiStaGetConfigImpl(const char *if_name, PlNetworkConfig *config);
PlErrCode PlWifiStaGetStatusImpl(const char *if_name, PlNetworkStatus *status);
PlErrCode PlWifiStaRegisterEventHandlerImpl(const char *if_name);
PlErrCode PlWifiStaUnregisterEventHandlerImpl(const char *if_name);
PlErrCode PlWifiStaStartImpl(const char *if_name);
PlErrCode PlWifiStaStopImpl(const char *if_name);
#else  // CONFIG_PL_NETWORK_WIFI_STATION
# define  PlWifiStaSetConfigImpl(if_name, config)       (kPlErrNoSupported)
# define  PlWifiStaGetConfigImpl(if_name, config)       (kPlErrNoSupported)
# define  PlWifiStaGetStatusImpl(if_name, status)       (kPlErrNoSupported)
# define  PlWifiStaRegisterEventHandlerImpl(if_name)    (kPlErrNoSupported)
# define  PlWifiStaUnregisterEventHandlerImpl(if_name)  (kPlErrNoSupported)
# define  PlWifiStaStartImpl(if_name)                   (kPlErrNoSupported)
# define  PlWifiStaStopImpl(if_name)                    (kPlErrNoSupported)
#endif  // CONFIG_PL_NETWORK_WIFI_STATION

#ifdef CONFIG_PL_NETWORK_WIFI_AP
// pl_wifi_ap_impl.c
PlErrCode PlWifiApSetConfigImpl(const char *if_name,
                                const PlNetworkConfig *config);
PlErrCode PlWifiApGetConfigImpl(const char *if_name, PlNetworkConfig *config);
PlErrCode PlWifiApGetStatusImpl(const char *if_name, PlNetworkStatus *status);
PlErrCode PlWifiApRegisterEventHandlerImpl(const char *if_name);
PlErrCode PlWifiApUnregisterEventHandlerImpl(const char *if_name);
PlErrCode PlWifiApStartImpl(const char *if_name);
PlErrCode PlWifiApStopImpl(const char *if_name);
#else  // CONFIG_PL_NETWORK_WIFI_AP
# define  PlWifiApSetConfigImpl(if_name, config)        (kPlErrNoSupported)
# define  PlWifiApGetConfigImpl(if_name, config)        (kPlErrNoSupported)
# define  PlWifiApGetStatusImpl(if_name, status)        (kPlErrNoSupported)
# define  PlWifiApRegisterEventHandlerImpl(if_name)     (kPlErrNoSupported)
# define  PlWifiApUnregisterEventHandlerImpl(if_name)   (kPlErrNoSupported)
# define  PlWifiApStartImpl(if_name)                    (kPlErrNoSupported)
# define  PlWifiApStopImpl(if_name)                     (kPlErrNoSupported)
#endif  // CONFIG_PL_NETWORK_WIFI_AP

#endif  // PL_WIFI_IMPL_H_
