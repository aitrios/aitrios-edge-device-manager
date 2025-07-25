/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_MAIN_MAIN_INTERNAL_H_
#define ESF_MAIN_MAIN_INTERNAL_H_

// include
#include <pthread.h>
#include <stdint.h>

// struct
// This structure defines the internal resources.
typedef struct EsfMainInfo {
  // Mutex object for internal state.
  pthread_mutex_t state_mutex;
  // Internal state.
  bool is_initialized;
  // Handle for UtilityMsg.
  int32_t utility_msg_handle;
  // Receive buffer for OSAL_MSG.
  void* recv_buf;
  // Maximum message size of OSAL_MSG.
  uint32_t max_msg_size;
} EsfMainInfo;

#endif  // ESF_MAIN_MAIN_INTERNAL_H_
