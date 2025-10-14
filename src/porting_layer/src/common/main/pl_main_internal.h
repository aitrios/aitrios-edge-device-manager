/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_MAIN_INTERNAL_H__
#define PL_MAIN_INTERNAL_H__

#if defined(__NuttX__)
#include <nuttx/config.h>
#endif
#include <stdio.h>

#include "pl_main.h"
#include "pl_main_table.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

// Macros ---------------------------------------------------------------------
#define EVENT_ID (0x9D00)
#define EVENT_ID_START (0x00)

#ifdef CONFIG_EXTERNAL_PL_MAIN_LOG
#define LOG_E(event_id, format, ...)                                      \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                   ##__VA_ARGS__);                                        \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_W(event_id, format, ...)                                     \
  WRITE_DLOG_WARN(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                  ##__VA_ARGS__);                                        \
  WRITE_ELOG_WARN(MODULE_ID_SYSTEM, (EVENT_ID | (EVENT_ID_START + event_id)));

#define LOG_I(format, ...)                                               \
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                  ##__VA_ARGS__);

#define LOG_D(format, ...)                                                \
  WRITE_DLOG_DEBUG(MODULE_ID_SYSTEM, "%s-%d:" format, __FILE__, __LINE__, \
                   ##__VA_ARGS__);

#else  // CONFIG_EXTERNAL_PL_MAIN_LOG
#define LOG_E(event_id, format, ...)                                           \
  printf("%s-%d:E:%d:%d:" format "\n", __FILE__, __LINE__,                     \
         (int)MODULE_ID_SYSTEM, (int)(EVENT_ID | (EVENT_ID_START + event_id)), \
         ##__VA_ARGS__)

#define LOG_W(event_id, format, ...)                                           \
  printf("%s-%d:W:%d:%d:" format "\n", __FILE__, __LINE__,                     \
         (int)MODULE_ID_SYSTEM, (int)(EVENT_ID | (EVENT_ID_START + event_id)), \
         ##__VA_ARGS__)

#define LOG_I(format, ...)                                                     \
  printf("%s-%d:I:%d:" format "\n", __FILE__, __LINE__, (int)MODULE_ID_SYSTEM, \
         ##__VA_ARGS__)

#define LOG_D(format, ...)                                                     \
  printf("%s-%d:D:%d:" format "\n", __FILE__, __LINE__, (int)MODULE_ID_SYSTEM, \
         ##__VA_ARGS__)

#endif  // CONFIG_EXTERNAL_PL_MAIN_LOG

// Functions ------------------------------------------------------------------
// """Formats the eMMC.

// This function performs the formatting process for the eMMC storage based on
// the provided device information.

// Args:
//     [IN] info: An array of device information structures.
//     [IN] info_size: The number of elements in the device information array.
//     [IN] cb: A callback function to be called during formatting.
//              If NULL, no callback will be invoked.
//     [IN] user_data: User data to be passed to the callback function.

// Returns:
//     PlErrCode: One of the PlErrCode values indicating the result of the
//                operation.
//     - kPlErrWrite: eMMC processing error.
//     - kPlErrInternal: Internal error.

// Note:
//     If multiple partitions are processed, the function will continue
//     processing all target partitions even if an error occurs.
//     The return value will be the error code of the last error that occurred.
// """
PlErrCode PlMainInternalEmmcFormat(const PlMainDeviceInformation* info,
                                   size_t info_size, PlMainKeepAliveCallback cb,
                                   void* user_data);

// """Mounts the eMMC.

// This function performs the mounting process for the eMMC storage based on
// the provided device information.

// Args:
//     [IN] info: An array of device information structures.
//     [IN] info_size: The number of elements in the device information array.

// Returns:
//     PlErrCode: One of the PlErrCode values indicating the result of the
//                operation.
//     - kPlErrWrite: eMMC processing error.
//     - kPlErrInternal: Internal error.

// Note:
//     If multiple partitions are processed, the function will continue
//     processing all target partitions even if an error occurs.
//     The return value will be the error code of the last error that occurred.
// """
PlErrCode PlMainInternalEmmcMount(const PlMainDeviceInformation* info,
                                  size_t info_size);

// """Unmounts the eMMC.

// This function performs the unmounting process for the eMMC storage based on
// the provided device information.

// Args:
//     [IN] info: An array of device information structures.
//     [IN] info_size: The number of elements in the device information array.

// Returns:
//     PlErrCode: One of the PlErrCode values indicating the result of the
//                operation.
//     - kPlErrWrite: eMMC processing error.
//     - kPlErrInternal: Internal error.

// Note:
//     If multiple partitions are processed, the function will continue
//     processing all target partitions even if an error occurs.
//     The return value will be the error code of the last error that occurred.
// """
PlErrCode PlMainInternalEmmcUnmount(const PlMainDeviceInformation* info,
                                    size_t info_size);

// """Formats the Flash memory.

// This function performs the formatting process for the Flash memory based on
// the provided device information.

// Args:
//     [IN] info: An array of device information structures.
//     [IN] info_size: The number of elements in the device information array.
//     [IN] cb: A callback function to be called during formatting.
//              If NULL, no callback will be invoked.
//     [IN] user_data: User data to be passed to the callback function.

// Returns:
//     PlErrCode: One of the PlErrCode values indicating the result of the
//                operation.
//     - kPlErrWrite: Flash processing error.
//     - kPlErrInternal: Internal error.

// Note:
//     If multiple partitions are processed, the function will continue
//     processing all target partitions even if an error occurs.
//     The return value will be the error code of the last error that occurred.
// """
PlErrCode PlMainInternalFlashFormat(const PlMainDeviceInformation* info,
                                    size_t info_size,
                                    PlMainKeepAliveCallback cb,
                                    void* user_data);

// """Mounts the Flash memory.

// This function performs the mounting process for the Flash memory based on
// the provided device information.

// Args:
//     [IN] info: An array of device information structures.
//     [IN] info_size: The number of elements in the device information array.

// Returns:
//     PlErrCode: One of the PlErrCode values indicating the result of the
//                operation.
//     - kPlErrWrite: Flash processing error.
//     - kPlErrInternal: Internal error.

// Note:
//     If multiple partitions are processed, the function will continue
//     processing all target partitions even if an error occurs.
//     The return value will be the error code of the last error that occurred.
// """
PlErrCode PlMainInternalFlashMount(const PlMainDeviceInformation* info,
                                   size_t info_size);

// """Unmounts the Flash memory.

// This function performs the unmounting process for the Flash memory based on
// the provided device information.

// Args:
//     [IN] info: An array of device information structures.
//     [IN] info_size: The number of elements in the device information array.

// Returns:
//     PlErrCode: One of the PlErrCode values indicating the result of the
//                operation.
//     - kPlErrWrite: Flash processing error.
//     - kPlErrInternal: Internal error.

// Note:
//     If multiple partitions are processed, the function will continue
//     processing all target partitions even if an error occurs.
//     The return value will be the error code of the last error that occurred.
// """
PlErrCode PlMainInternalFlashUnmount(const PlMainDeviceInformation* info,
                                     size_t info_size);

// """Determines if a specified feature is supported.

// This function checks whether a given feature is available for use based on
// the provided list of supported features.

// Args:
//     [IN] support: An array of supported features.
//     [IN] support_size: The number of elements in the support array.
//     [IN] type: The feature to be checked.

// Returns:
//     PlErrCode: kPlErrCodeOk if the feature is supported,
//                kPlErrNoSupported if not supported,
//                or other PlErrCode values for errors.
//     - kPlErrInvalidParam: Parameter error.
//     - kPlErrNoSupported: Unsupported feature.
//     - kPlErrInternal: Internal error.

// Note:
// """
PlErrCode PlMainInternalIsFeatureSupported(const PlMainFeatureType* support,
                                           size_t support_size,
                                           PlMainFeatureType type);

#endif  // PL_MAIN_INTERNAL_H__
