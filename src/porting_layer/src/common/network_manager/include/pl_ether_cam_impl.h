/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_ETHER_CAM_IMPL_H_
#define PL_ETHER_CAM_IMPL_H_

// Includes --------------------------------------------------------------------
#include <stdbool.h>
#include <stdint.h>

#include "pl.h"
#include "pl_network.h"

// Types (typedef / enum / struct / union) -------------------------------------
typedef struct {
  int32_t port;
  bool is_ioexp;
  bool is_active_high;
} EtherPortInfo;

typedef struct {
  char if_name[PL_NETWORK_IFNAME_LEN];
  int32_t spi_port;
  EtherPortInfo reset;
  EtherPortInfo irq;
  EtherPortInfo power;
} EtherDevice;

// Public functions ------------------------------------------------------------
PlErrCode PlEtherInitializeCamImpl(const char *if_name);
PlErrCode PlEtherFinalizeCamImpl(const char *if_name);

#endif  // PL_ETHER_CAM_IMPL_H_
