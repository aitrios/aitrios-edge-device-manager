/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include "netutils/dhcpc.h"

FAR void *dhcpc_open(FAR const char *interface,
                     FAR const void *mac_addr, int mac_len)
{
	return NULL;
}

int  dhcpc_request(FAR void *handle, FAR struct dhcpc_state *presult)
{
	return 0;
}

int  dhcpc_request_async(FAR void *handle, dhcpc_callback_t callback)
{
	return 0;
}

void dhcpc_cancel(FAR void *handle)
{
}

void dhcpc_close(FAR void *handle)
{
}
