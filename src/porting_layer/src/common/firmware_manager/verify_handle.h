/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_PORTING_LAYER_FIRMWARE_MANAGER_VERIFY_HANDLE_H_
#define ESF_PORTING_LAYER_FIRMWARE_MANAGER_VERIFY_HANDLE_H_

/// @brief Verify `handle` is valid.
/// @param handle [in]
/// @return true: `handle` is valid, false: invalid
#define FW_MGR_PL_VERIFY_HANDLE(handle) \
  (((void *)handle == (void *)s_active_context) && (handle != NULL))

#endif  // ESF_PORTING_LAYER_FIRMWARE_MANAGER_VERIFY_HANDLE_H_
