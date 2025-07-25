/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <netutils/ntpclient.h>

int ntpc_start_with_list(FAR const char *ntp_server_list)
{
	return 0;
}

int ntpc_start_with_params(FAR const char *ntp_server_list,
                           FAR const struct ntp_sync_params_s *params)
{
	return 0;
}

int ntpc_start(void)
{
	return 0;
}

int ntpc_stop(void)
{
	return 0;
}

int ntpc_status(struct ntpc_status_s *statusp)
{
	return 0;
}
