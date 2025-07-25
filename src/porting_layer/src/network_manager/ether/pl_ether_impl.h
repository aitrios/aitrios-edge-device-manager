/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_ETHER_IMPL_H_
#define PL_ETHER_IMPL_H_

// Includes --------------------------------------------------------------------


// Macros ----------------------------------------------------------------------


// Typdefs ---------------------------------------------------------------------
// Ethernet Port Info
typedef struct {
  int32_t         port;
  bool            is_ioexp;
  bool            is_active_high;
} EtherPortInfo;

// Ethernet Device
typedef struct {
  char            if_name[PL_NETWORK_IFNAME_LEN];
//  char            name[NETWORK_DEVNAME_LEN];
//  bool            enable;
  int32_t         spi_port;
  EtherPortInfo   reset;
  EtherPortInfo   irq;
  EtherPortInfo   power;
} EtherDevice;

// Functions -------------------------------------------------------------------
#ifdef CONFIG_PL_ETH0_HAVE_DEVICE
PlErrCode PlEtherInitializeImpl(const char *if_name);
PlErrCode PlEtherFinalizeImpl(const char *if_name);
#else  // CONFIG_PL_ETH0_HAVE_DEVICE
# define  PlEtherInitializeImpl(if_name)    (kPlErrCodeOk)
# define  PlEtherFinalizeImpl(if_name)      (kPlErrCodeOk)
#endif  // CONFIG_PL_ETH0_HAVE_DEVICE

#endif  // PL_ETHER_IMPL_H_
