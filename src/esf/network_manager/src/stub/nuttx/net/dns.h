/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INCLUDE_NUTTX_NET_DNS_H
#define __INCLUDE_NUTTX_NET_DNS_H

/****************************************************************************
 * Name: dns_default_nameserver
 *
 * Description:
 *   Reset the resolver to use only the default DNS server, if any.
 *
 ****************************************************************************/

int dns_default_nameserver(void);
#endif /* __INCLUDE_NUTTX_NET_DNS_H */
