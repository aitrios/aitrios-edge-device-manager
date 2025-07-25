/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_LOG_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_LOG_H_

#include "utility_log.h"
#include "utility_log_module_id.h"

#define ELOG_SOCKET_ERROR (0x9609)        // Same number as pl_network.c
#define ELOG_SIOCSIFFLAGS_ERROR (0x960A)  // Same number as pl_network.c
#define ELOG_SIOCGIFFLAGS_ERROR (0x960B)  // Same number as pl_network.c
#define ELOG_OS_ERROR (0x960C)            // Same number as pl_network.c
#define ELOG_SIOCGMIIPHY_ERROR (0x9661)   // Same number as pl_ether.c
#define ELOG_SIOCGMIIREG_ERROR (0x9662)   // Same number as pl_ether.c
#define ELOG_LIBNM_ERROR (0x96F7)
// 0x96F8 - 0x96FF is reserved

#define BASE (MODULE_ID_SYSTEM)
#define DLOGE(fmt, ...)                                                      \
  do {                                                                       \
    WRITE_DLOG_ERROR(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGW(fmt, ...)                                                     \
  do {                                                                      \
    WRITE_DLOG_WARN(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DLOGD(fmt, ...)                                                      \
  do {                                                                       \
    WRITE_DLOG_DEBUG(BASE, "%s-%d:" fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)

#define ELOGE(event_id)               \
  do {                                \
    WRITE_ELOG_ERROR(BASE, event_id); \
  } while (0)

#define ELOGW(event_id)              \
  do {                               \
    WRITE_ELOG_WARN(BASE, event_id); \
  } while (0)

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_UTIL_LOG_H_
