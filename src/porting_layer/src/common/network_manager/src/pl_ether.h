/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_ETHER_H_
#define PL_ETHER_H_

// Includes --------------------------------------------------------------------
#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"

// Public functions ------------------------------------------------------------
#ifdef CONFIG_PL_NETWORK_HAVE_ETHER
PlErrCode PlEtherInitialize(struct network_info *net_info,
                            uint32_t thread_priority);
PlErrCode PlEtherFinalize(struct network_info *net_info);
PlErrCode PlEtherSetConfig(struct network_info *net_info,
                           const PlNetworkConfig *config);
PlErrCode PlEtherGetConfig(struct network_info *net_info,
                           PlNetworkConfig *config);
PlErrCode PlEtherGetStatus(struct network_info *net_info,
                           PlNetworkStatus *status);
PlErrCode PlEtherRegisterEventHandler(struct network_info *net_info);
PlErrCode PlEtherUnregisterEventHandler(struct network_info *net_info);
PlErrCode PlEtherStart(struct network_info *net_info);
PlErrCode PlEtherStop(struct network_info *net_info);
#else
#define PlEtherInitialize(net_info, thread_priority) (kPlErrNoSupported)
#define PlEtherFinalize(net_info) (kPlErrNoSupported)
#define PlEtherSetConfig(net_info, config) (kPlErrNoSupported)
#define PlEtherGetConfig(net_info, config) (kPlErrNoSupported)
#define PlEtherGetStatus(net_info, status) (kPlErrNoSupported)
#define PlEtherRegisterEventHandler(net_info) (kPlErrNoSupported)
#define PlEtherUnregisterEventHandler(net_info) (kPlErrNoSupported)
#define PlEtherStart(net_info) (kPlErrNoSupported)
#define PlEtherStop(net_info) (kPlErrNoSupported)
#endif

#endif  // PL_ETHER_H_
