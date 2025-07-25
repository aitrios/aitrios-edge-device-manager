/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdk_backdoor.h"

static bool stub_backdoor_get_agent_status_disconnected = false;

enum evp_agent_status EVP_getAgentStatus(void) {
    if (stub_backdoor_get_agent_status_disconnected) {
        return EVP_AGENT_STATUS_DISCONNECTED;
    }
    return EVP_AGENT_STATUS_CONNECTED;
}

void StubSetupDisconnectedStatus(void) {
  stub_backdoor_get_agent_status_disconnected = true;
}

void StubClearBackdoorParameter(void) {
  stub_backdoor_get_agent_status_disconnected = false;
}
