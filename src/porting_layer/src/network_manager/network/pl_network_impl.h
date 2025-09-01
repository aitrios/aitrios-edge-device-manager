/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_IMPL_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_IMPL_H_

#include "pl.h"
#include "pl_network.h"

#ifdef __NuttX__
#include <sched.h>
#include <sys/queue.h>
#else
#include <bsd/sys/queue.h>
typedef int irqstate_t;
#endif

irqstate_t NetworkLock(void);
void NetworkUnlock(irqstate_t flags);

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_IMPL_H_
