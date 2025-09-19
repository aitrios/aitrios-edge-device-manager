/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_NETWORK_UTIL_H_
#define PL_NETWORK_UTIL_H_

// Includes --------------------------------------------------------------------
#include <stdint.h>

#include "pl_network.h"

// Types (typedef / enum / struct / union) -------------------------------------
typedef unsigned int PlNetworkUtilIrqstate;

// Public functions ------------------------------------------------------------
PlNetworkUtilIrqstate PlNetworkUtilNetworkLock(void);
void PlNetworkUtilNetworkUnlock(PlNetworkUtilIrqstate flags);
void PlNetworkUtilSchedLock(void);
void PlNetworkUtilSchedUnlock(void);
PlErrCode PlNetworkUtilGetNetStat(char *buf, const uint32_t buf_size);
PlErrCode PlNetworkSetIfStatus(const char *ifname, const bool is_ifup);
PlErrCode PlNetworkGetIfStatus(const char *ifname, bool *is_ifup);
PlErrCode PlNetworkGetLinkStatus(const char *ifname, bool *is_linkup,
                                 bool *is_phy_id_valid);

#endif  // PL_NETWORK_UTIL_H_
