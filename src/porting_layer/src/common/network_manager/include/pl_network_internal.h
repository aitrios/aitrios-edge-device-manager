/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_NETWORK_INTERNAL_H_
#define PL_NETWORK_INTERNAL_H_

// Types (typedef / enum / struct / union) -------------------------------------
// Network Interface State
typedef enum {
  kPlNetIfStateStopped = 0,
  kPlNetIfStateStarted,
  kPlNetIfStateMax
} PlNetIfState;

struct network_info;

// PL Network Operations
typedef struct {
  PlErrCode (*set_config)(struct network_info *info,
                          const PlNetworkConfig *config);
  PlErrCode (*get_config)(struct network_info *info, PlNetworkConfig *config);
  PlErrCode (*get_status)(struct network_info *info, PlNetworkStatus *status);
  PlErrCode (*reg_event)(struct network_info *info);
  PlErrCode (*unreg_event)(struct network_info *info);
  PlErrCode (*start)(struct network_info *info);
  PlErrCode (*stop)(struct network_info *info);
} PlNetworkOps;

// PL Network Information
typedef struct network_info {
  char if_name[PL_NETWORK_IFNAME_LEN];
  PlNetworkType if_type;
  PlNetIfState state;
  PlNetworkEventHandler handler;
  void *private_data;
  PlNetworkOps *ops;
  void *if_info;
} PlNetworkInfo;

// Public functions ------------------------------------------------------------
PlErrCode PlNetworkEventSend(const char *if_name,
                             PlNetworkEvent network_event_id, uint8_t reason);

#endif  // PL_NETWORK_INTERNAL_H_
