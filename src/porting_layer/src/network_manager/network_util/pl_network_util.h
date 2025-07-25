/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_H_

#include "pl_network.h"

PlErrCode PlNetworkSetIfStatus(const char *ifname, const bool is_ifup);
PlErrCode PlNetworkGetIfStatus(const char *ifname, bool *is_ifup);
PlErrCode PlNetworkGetLinkStatus(const char *ifname, bool *is_linkup,
                                 bool *is_phy_id_valid);

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_H_
