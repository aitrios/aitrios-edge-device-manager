/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WAMR_APP_NATIVE_EXPORT_MEMORY_MANAGER_API_H_
#define WAMR_APP_NATIVE_EXPORT_MEMORY_MANAGER_API_H_

#include <stdint.h>

#include "wasm_export.h"

uint32_t EsfMemoryManagerPread_wasm(wasm_exec_env_t exec_env, uint32_t handle,
                                    void *buf, uint32_t sz, uint64_t offset,
                                    uint32_t *resultp);

#endif  // WAMR_APP_NATIVE_EXPORT_MEMORY_MANAGER_API_H_
