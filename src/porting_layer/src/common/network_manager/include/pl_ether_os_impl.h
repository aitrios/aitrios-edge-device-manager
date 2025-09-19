/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_ETHER_OS_IMPL_H_
#define PL_ETHER_OS_IMPL_H_

// Includes --------------------------------------------------------------------
#include <pthread.h>
#include <semaphore.h>

#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"

// Types (typedef / enum / struct / union) -------------------------------------
// Ethernet Statement
typedef enum {
  kPlEtherStandby = 0,
  kPlEtherReady,
  kPlEtherRunning,
  kPlEtherStateMax
} PlEtherState;

// Ethernet info
typedef struct {
  char if_name[PL_NETWORK_IFNAME_LEN];
  pthread_mutex_t mutex;
  PlEtherState state;
  sem_t sem;
  pthread_t pid;
  bool is_thread;
  bool is_if_up;
  bool is_link_up;
  bool is_phy_id_valid;
} PlEtherInfo;

// Public functions ------------------------------------------------------------
PlErrCode PlEtherInitializeOsImpl(const char *if_name);
PlErrCode PlEtherFinalizeOsImpl(const char *if_name);
PlErrCode PlEtherRegisterEventHandlerOsImpl(struct network_info *net_info);

#endif  // PL_ETHER_OS_IMPL_H_
