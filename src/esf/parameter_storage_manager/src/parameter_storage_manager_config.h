/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_CONFIG_H_
#define ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_CONFIG_H_

#ifdef __NuttX__
#include <nuttx/config.h>
#else
#define FAR
#endif

#if !defined(CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX)
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX is not defined.
#elif CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX <= 0
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_HANDLE_MAX must be a positive integer.  // NOLINT
#endif

#if !defined(CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS)
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS is not defined.
#elif CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS <= 0
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_TIMEOUT_MS must be a positive integer.  // NOLINT
#endif

#if !defined(CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX)
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX is not defined.  // NOLINT
#elif CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX <= 0
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_FACTORY_RESET_MAX must be a positive integer.  // NOLINT
#endif

#if !defined(CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX)
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX is not defined.
#elif CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX <= 0
#error CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_UPDATE_MAX must be a positive integer.  // NOLINT
#endif

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_PL_STORAGE_STUB
#define ESF_PARAMETER_STORAGE_MANAGER_PL_STORAGE_FILE \
  "parameter_storage_manager/src/stub/include/pl/pl_storage.h"
#else
#define ESF_PARAMETER_STORAGE_MANAGER_PL_STORAGE_FILE "pl_storage.h"
#endif

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_MEMORY_MANAGER_STUB
#define ESF_PARAMETER_STORAGE_MANAGER_UTILITY_MEMORY_FILE \
  "parameter_storage_manager/src/stub/include/memory_manager/memory_manager.h"
#else
#define ESF_PARAMETER_STORAGE_MANAGER_UTILITY_MEMORY_FILE "memory_manager.h"
#endif

#ifdef CONFIG_EXTERNAL_PARAMETER_STORAGE_MANAGER_POWER_MANAGER_STUB
#define ESF_PARAMETER_STORAGE_MANAGER_POWER_MANAGER_FILE \
  "parameter_storage_manager/src/stub/include/power_manager/power_manager.h"
#define EsfPwrMgrWdtKeepAlive PSM_EsfPwrMgrWdtKeepAlive
#else
#define ESF_PARAMETER_STORAGE_MANAGER_POWER_MANAGER_FILE "power_manager.h"
#endif

#include "parameter_storage_manager/src/parameter_storage_manager_event_log.h"
#include "parameter_storage_manager/src/parameter_storage_manager_log.h"

#endif  // ESF_PARAMETER_STORAGE_MANAGER_SRC_PARAMETER_STORAGE_MANAGER_CONFIG_H_
