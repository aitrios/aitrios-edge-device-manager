/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_ACCESSOR_PARAMETER_STORAGE_MANAGER_H_
#define ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_ACCESSOR_PARAMETER_STORAGE_MANAGER_H_

#include "network_manager.h"

// """Saves network connection information via ParameterStorageManager.

// Saves network connection information via ParameterStorageManager.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure for network connection information.
//       Only data for which the mask setting is enabled will be saved.
//       Do not specify NULL.
//     data (const EsfNetworkManagerParameter *):
//       Network connection information.
//       Please set the information to be saved for data with mask settings
//       enabled. Do not specify NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultExternalError:
//       Error response from ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultResourceExhausted: Memory allocation failed.
EsfNetworkManagerResult
EsfNetworkManagerParameterStorageManagerAccessorSaveNetwork(
    const EsfNetworkManagerParameterMask *mask,
    const EsfNetworkManagerParameter *data);

// """Load network connection information via ParameterStorageManager.

// Load network connection information via ParameterStorageManager.

// Args:
//     mask (const EsfNetworkManagerParameterMask *):
//       This is a mask structure for network connection information.
//       Only data for which mask settings are enabled will be retrieved.
//       Do not specify NULL.
//     data (EsfNetworkManagerParameter *):
//       This is the acquired information.
//       Do not specify NULL.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultExternalError:
//       Error response from ParameterStorageManager API.
//     kEsfNetworkManagerResultInvalidParameter: Input parameter error.
//     kEsfNetworkManagerResultResourceExhausted: Memory allocation failed.
EsfNetworkManagerResult
EsfNetworkManagerParameterStorageManagerAccessorLoadNetwork(
    const EsfNetworkManagerParameterMask *mask,
    EsfNetworkManagerParameter *data);

#endif  // ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_ACCESSOR_PARAMETER_STORAGE_MANAGER_H_
