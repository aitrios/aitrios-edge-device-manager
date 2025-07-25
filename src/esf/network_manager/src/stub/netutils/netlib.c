/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "netutils/netlib.h"

int netlib_get_ipv4addr(FAR const char *ifname, FAR struct in_addr *addr)
{
	return 0;
}

int netlib_set_ipv4addr(FAR const char *ifname,
                        FAR const struct in_addr *addr)
{
	return 0;
}

int netlib_set_dripv4addr(FAR const char *ifname,
                          FAR const struct in_addr *addr)
{
	return 0;
}

int netlib_get_dripv4addr(FAR const char *ifname, FAR struct in_addr *addr)
{
	return 0;
}

int netlib_set_ipv4netmask(FAR const char *ifname,
                           FAR const struct in_addr *addr)
{
	return 0;
}

int netlib_get_ipv4netmask(FAR const char *ifname, FAR struct in_addr *addr)
{
	return 0;
}

int netlib_ipv4adaptor(in_addr_t destipaddr, FAR in_addr_t *srcipaddr)
{
	return 0;
}

int netlib_set_ipv4dnsaddr(FAR const struct in_addr *inaddr)
{
	return 0;
}

int netlib_setmacaddr(FAR const char *ifname, FAR const uint8_t *macaddr)
{
	return 0;
}

int netlib_getmacaddr(FAR const char *ifname, FAR uint8_t *macaddr)
{
	return 0;
}
