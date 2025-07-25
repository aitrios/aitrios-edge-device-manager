/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_LOG_MANAGER_METRICS_H_
#define ESF_LOG_MANAGER_METRICS_H_

#include "log_manager.h"
#include "log_manager_internal.h"

#define ESF_LOG_MANAGER_METRICS_INVALID_THREAD_ID (-1)

// """ Initialize metrics
// Args:
//    none
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
EsfLogManagerStatus EsfLogManagerInitializeMetrics(void);

// """ Deinitialize metrics
// Args:
//    none
// Returns:
//    kEsfLogManagerStatusOk: success
//    kEsfLogManagerStatusFailed: abnormal termination
EsfLogManagerStatus EsfLogManagerDeinitMetrics(void);

#endif  // ESF_LOG_MANAGER_METRICS_H_
