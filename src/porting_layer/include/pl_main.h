/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef PL_MAIN_H__
#define PL_MAIN_H__

#include "pl.h"

// Typedef ---------------------------------------------------------------------
// An enumeration representing the list of functions in Main.
typedef enum PlMainFeatureType {
  PlMainFeatureReboot,
  PlMainFeatureShutdown,
  PlMainFeatureFactoryReset,
  PlMainFeatureDowngrade,
  PlMainFeatureMax,
} PlMainFeatureType;

// Type of callback function to call KeepAlive during formatting.
typedef void (*PlMainKeepAliveCallback)(void*);

// Public API-------------------------------------------------------------------

// """Formats the eMMC.

// This function performs the formatting process for the eMMC storage.

// Args:
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
PlErrCode PlMainEmmcFormat(PlMainKeepAliveCallback cb, void* user_data);

// """Mounts the eMMC.

// This function performs the mounting process for the eMMC storage.

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
PlErrCode PlMainEmmcMount(void);

// """Unmounts the eMMC.

// This function performs the unmounting process for the eMMC storage.

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
PlErrCode PlMainEmmcUnmount(void);

// """Formats the Flash memory.

// This function performs the formatting process for the Flash memory.

// Args:
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
PlErrCode PlMainFlashFormat(PlMainKeepAliveCallback cb, void* user_data);

// """Mounts the Flash memory.

// This function performs the mounting process for the Flash memory.

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
PlErrCode PlMainFlashMount(void);

// """Unmounts the Flash memory.

// This function performs the unmounting process for the Flash memory.

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
PlErrCode PlMainFlashUnmount(void);

// """Determines if a specified feature is supported.

// This function checks whether a given feature is available for use.

// Args:
//     [IN] type: An enumeration representing the Main feature list.

// Returns:
//     PlErrCode: kPlErrCodeOk if the feature is supported,
//                kPlErrNoSupported if not supported,
//                or other PlErrCode values for errors.
//     - kPlErrInvalidParam: Parameter error.
//     - kPlErrNoSupported: Unsupported feature.
//     - kPlErrInternal: Internal error.

// Note:
//     For T5:
//         - Downgrade (PlMainFeatureDowngrade) is not supported.
//     For T3:
//         - All features are supported.
// """
PlErrCode PlMainIsFeatureSupported(PlMainFeatureType type);

#endif /* PL_MAIN_H__ */
