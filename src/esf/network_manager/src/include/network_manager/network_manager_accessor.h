/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_ACCESSOR_H_
#define ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_ACCESSOR_H_

#include <stdbool.h>
#include <stdint.h>

#include "led_manager.h"
#include "network_manager.h"
#include "network_manager/network_manager_internal.h"
#include "network_manager/network_manager_resource.h"

// Led Manager setting state enumeration definition.
typedef enum EsfNetworkManagerAccessorLedManagerStatus {
  // No led manager operation.
  kEsfNetworkManagerAccessorLedManagerStatusIgnore = 0,

  // Network initialize error.
  kEsfNetworkManagerAccessorLedManagerStatusInitializeError,

  // Network deinitialize error.
  kEsfNetworkManagerAccessorLedManagerStatusDeinitializeError,

  // Network start error.
  kEsfNetworkManagerAccessorLedManagerStatusStartError,

  // Network stop error.
  kEsfNetworkManagerAccessorLedManagerStatusStopError,

  // Connection parameter error.
  kEsfNetworkManagerAccessorLedManagerStatusParameterError,

  // LoadingSSID.
  kEsfNetworkManagerAccessorLedManagerStatusLoadingSSID,

  // Connecting.
  kEsfNetworkManagerAccessorLedManagerStatusConnecting,

  // Connected.
  kEsfNetworkManagerAccessorLedManagerStatusConnected,

  // Disconnecting.
  kEsfNetworkManagerAccessorLedManagerStatusDisconnecting,

  // Disconnected.
  kEsfNetworkManagerAccessorLedManagerStatusDisconnected
} EsfNetworkManagerAccessorLedManagerStatus;

// """Get system information from HAL.

// Gets the system reception held by HAL.

// Args:
//     info_total_num (uint32_t *): Number of acquired system information.
//     infos (PlNetworkSystemInfo **):
//       This is the acquired system information array.
//       It responds with HAL information as is.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: Error response from HAL API.
//     kEsfNetworkManagerResultInternalError: Internal error.

EsfNetworkManagerResult EsfNetworkManagerAccessorGetSystemInfo(
    uint32_t *info_total_num, PlNetworkSystemInfo **infos);

// """Get IF connection state from HAL.

// Gets the IF state from HAL and sets it in the IF connection information.

// Args:
//     if_name (const char *): IF name to get the status.
//     status (EsfNetworkManagerStatusInfo *):Obtained IF connection status.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: Error response from HAL API.
//     kEsfNetworkManagerResultInternalError: Internal error.
EsfNetworkManagerResult EsfNetworkManagerAccessorGetStatusInfo(
    const char *if_name, EsfNetworkManagerStatusInfo *status);

#ifndef CONFIG_EXTERNAL_NETWORK_MANAGER_DISABLE
EsfNetworkManagerResult EsfNetworkManagerAccessorGetIFInfo(
    const EsfNetworkManagerModeInfo *mode, EsfNetworkManagerIPInfo *ip_info);
#endif

// """Get IF RSSI from HAL.

// Get IF state from HAL and get RSSI information.

// Args:
//     if_name (const char *): IF name to get the RSSI.
//     rssi (int8_t *):This is the RSSI obtained.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: Error response from HAL API.
//     kEsfNetworkManagerResultInternalError: Internal error.
EsfNetworkManagerResult EsfNetworkManagerAccessorGetRssi(const char *if_name,
                                                         int8_t *rssi);

// """Get netstat information from HAL.

// Get netstat information from HAL.

// Args:
//     netstat_buf_size (int32_t): Specify the size of netstat_buf.
//     netstat_buf (char *): Specify the buffer for storing
//       the Netstat information acquisition results.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: Error response from HAL API.
//     kEsfNetworkManagerResultInternalError: Internal error.
EsfNetworkManagerResult EsfNetworkManagerAccessorGetNetstat(
    const int32_t netstat_buf_size, char *netstat_buf);

// """Unregisters a HAL event handler.

// Unregisters the HAL event handler for the specified IF.

// Args:
//     if_name_ether (const char *): IF name for HAL event handler
//       registration cancellation (Normal mode IF type Ether).
//       If NULL, registration cancellation processing will not be performed.

//     if_name_wifist (const char *): IF name for HAL event handler
//       registration cancellation (Normal mode IF type WiFi).
//       If NULL, registration cancellation processing will not be performed.

//     if_name_wifiap (const char *): IF name for HAL event handler
//       registration cancellation (AccessPoint mode).
//       If NULL, registration cancellation processing will not be performed.
void EsfNetworkManagerAccessorUnregisterAllEventHandler(
    const char *if_name_ether, const char *if_name_wifist,
    const char *if_name_wifiap);

// """Stop all networks.

// Stop all networks at PlNetwork.

// Args:
//     if_name_ether (const char *): IF name.

//     if_name_wifist (const char *): IF name.

//     if_name_wifiap (const char *): IF name.
void EsfNetworkManagerAccessorStopAllNetwork(const char *if_name_ether,
                                             const char *if_name_wifist,
                                             const char *if_name_wifiap);

// """Performs network connection start processing.

// Starts a network connection for the IF of the specified handle
// with the specified connection information.
// Use HAL to register the event handler for the I/F that corresponds
// to the connection mode.
// Use HAL to set network information for the I/F that corresponds
// to the connection mode.
// Start the network connection of the I/F corresponding
// to the connection mode using HAL.

// Args:
//     connect_info (const EsfNetworkManagerOSInfo *):
//       Network connection information used to initiate a connection.
//     handle_internal (EsfNetworkManagerHandleInternal *):
//       This is the actual status of the handle to start the connection.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: Error response from HAL API.
//     kEsfNetworkManagerResultUtilityDHCPServerError:
//       Error response from DHCP server operation API
//     kEsfNetworkManagerResultUtilityIPAddressError:
//       Error response from IP address manipulation API
//     kEsfNetworkManagerResultInternalError: Internal error.
EsfNetworkManagerResult EsfNetworkManagerAccessorStart(
    const EsfNetworkManagerOSInfo *connect_info,
    EsfNetworkManagerHandleInternal *handle_internal);

// """Performs network connection termination processing.

// Begins a network shutdown of the IF for the specified handle.
// Stop the DHCP server if it was running in AccessPoint mode.
// Use HAL to start a network stop for the I/F that corresponds
// to the connection mode.

// Args:
//     handle_internal (EsfNetworkManagerHandleInternal *): This is the actual
//     status
//       of the handle whose connection is to be stopped.

// Returns:
//     EsfNetworkManagerResult: The code returns one of the values
//     EsfNetworkManagerResult depending on the execution result.

// Yields:
//     kEsfNetworkManagerResultSuccess: Success.
//     kEsfNetworkManagerResultHWIFError: Error response from HAL API.
//     kEsfNetworkManagerResultUtilityDHCPServerError:
//       Error response from DHCP server operation API
//     kEsfNetworkManagerResultInternalError: Internal error.
EsfNetworkManagerResult EsfNetworkManagerAccessorStop(
    EsfNetworkManagerHandleInternal *handle_internal);

// """Set the state to the led manager.

// The specified state will be converted to
// an LedManager state and set as an LedManager.
// Even if the setting fails, no error will be returned
// and only a log will be output.

// Args:
//     mode (EsfNetworkManagerMode): Network mode.
//     status (EsfNetworkManagerAccessorLedManagerStatus): Set status.
//     netif_kind (EsfNetworkManagerNetifKind): Network Interface Kind.
void EsfNetworkManagerAccessorSetLedManagerStatus(
    EsfNetworkManagerMode mode,
    EsfNetworkManagerAccessorLedManagerStatus status,
    EsfNetworkManagerNetifKind netif_kind);

// """Set the service led state to the led manager.

// Set the service led state to the led manager.
// Even if the setting fails, no error will be returned
// and only a log will be output.

// Args:
//     status (EsfLedManagerLedStatus): Set status.
//     enabled (bool): status enabled.
void EsfNetworkManagerAccessorSetLedManagerStatusService(
    EsfLedManagerLedStatus status, bool enabled);

// """Calls the user registration callback function.

// Calls the user registration callback function.

// Args:
//     mode (EsfNetworkManagerMode): Network mode.
//     mode_info (const EsfNetworkManagerModeInfo *):
//       Mode information resource pointer.
void EsfNetworkManagerAccessorCallCallback(
    EsfNetworkManagerMode mode, const EsfNetworkManagerModeInfo *mode_info);

// """Initialize the HAL.

// Initialize the HAL.
void EsfNetworkManagerAccessorInit(void);

// """Finalize the HAL.

// Finalize the HAL.
void EsfNetworkManagerAccessorDeinit(void);

#endif  // ESF_NETWORK_MANAGER_SRC_INCLUDE_NETWORK_MANAGER_NETWORK_MANAGER_ACCESSOR_H_
