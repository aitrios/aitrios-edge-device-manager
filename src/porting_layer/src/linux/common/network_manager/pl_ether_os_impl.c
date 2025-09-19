/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Includes --------------------------------------------------------------------
#include "pl_ether_os_impl.h"

#include "pl.h"
#include "pl_network.h"
#include "pl_network_internal.h"
#include "pl_network_log.h"

// Public functions ------------------------------------------------------------
PlErrCode PlEtherInitializeOsImpl(const char *if_name) {
  // Do nothing
  (void)if_name;
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlEtherFinalizeOsImpl(const char *if_name) {
  // Do nothing
  (void)if_name;
  return kPlErrCodeOk;
}

// -----------------------------------------------------------------------------
PlErrCode PlEtherRegisterEventHandlerOsImpl(struct network_info *net_info) {
  // Notifies the upper layer of the current state as an event.
  PlEtherInfo *info = (PlEtherInfo *)net_info->if_info;
  PlErrCode err = kPlErrCodeError;
  uint8_t reason = 0;
  PlNetworkEvent event = kPlNetworkEventMax;

  event = kPlNetworkEventIfDown;
  if (info->is_if_up) {
    event = kPlNetworkEventIfUp;
  }
  err = PlNetworkEventSend(info->if_name, event, reason);
  DLOGI("%s:%d:%d", __func__, err, event);

  event = kPlNetworkEventLinkDown;
  if (info->is_link_up) {
    event = kPlNetworkEventLinkUp;
  }
  err = PlNetworkEventSend(info->if_name, event, reason);
  DLOGI("%s:%d:%d", __func__, err, event);
  return kPlErrCodeOk;
}
