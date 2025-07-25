/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APPS_INCLUDE_NETUTILS_DHCPD_H
#define __APPS_INCLUDE_NETUTILS_DHCPD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <netinet/in.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

int dhcpd_run(FAR const char *interface);
int dhcpd_start(FAR const char *interface);
int dhcpd_stop(void);
int dhcpd_set_startip(in_addr_t startip);
int dhcpd_set_routerip(in_addr_t routerip);
int dhcpd_set_netmask(in_addr_t netmask);
int dhcpd_set_dnsip(in_addr_t dnsip);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_DHCPD_H */
