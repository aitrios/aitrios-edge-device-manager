/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "led_manager.h"

EsfLedManagerResult EsfLedManagerInit(void) { return kEsfLedManagerSuccess; }
EsfLedManagerResult EsfLedManagerDeinit(void) { return kEsfLedManagerSuccess; }
EsfLedManagerResult EsfLedManagerSetStatus(
    const EsfLedManagerLedStatusInfo* status) {
  printf(
      "[STUB] EsfLedManagerSetStatus led=%d status=%d enabled=%d\n",
      status->led, status->status, status->enabled);
  return kEsfLedManagerSuccess;
}
