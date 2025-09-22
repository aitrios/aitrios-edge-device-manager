/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_CLOCK_MANAGER_H_
#define PL_CLOCK_MANAGER_H_

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#define PL_CLOCK_MANAGER_INVALID_THREAD_ID ((pthread_t)(-1))

// This enumeration represents enumeration-constants which Clock Manager's APIs
// return.
typedef enum {
  kPlClockManagerSuccess,               // Success
  kPlClockManagerParamError,            // Invalid parameter error
  kPlClockManagerInternalError,         // Internal error
  kPlClockManagerStateTransitionError,  // State translation error
  kPlClockManagerNotifierHasAlreadyFinished,
  kPlClockManagerMonitorHasAlreadyFinished,
  kPlClockManagerDaemonHasAlreadyFinished
} PlClockManagerReturnValue;

typedef enum PlClockManagerNtpTimeSyncStatus {
  kPlClockManagerNtpTimeSyncFailure,
  kPlClockManagerNtpTimeSyncSuccess,
  kPlClockManagerHasNotCompletedYet
} PlClockManagerNtpTimeSyncStatus;

typedef struct PlClockManagerMutexBase {
  pthread_mutex_t m_mutex;
} PlClockManagerMutexBase;

typedef struct PlClockManagerCondBase {
  pthread_mutex_t m_mutex;  // mutex
  pthread_cond_t m_cond;    // condition
} PlClockManagerCondBase;

typedef enum PlClockManagerParamType {
  kPlClockManagerParamTypeOff,
  kPlClockManagerParamTypeDefault,
  kPlClockManagerParamTypeCustom,
  kPlClockManagerParamTypeNumMax
} PlClockManagerParamType;

// The maximum size of a host name or an IPv4 address for an NTP server.
// This size includes a terminal null character (i.e., '\0').
#define PL_CLOCK_MANAGER_NTPADDR_MAX_SIZE (272)
typedef struct PlClockManagerConnection {
  char hostname[PL_CLOCK_MANAGER_NTPADDR_MAX_SIZE];
} PlClockManagerConnection;

typedef struct PlClockManagerCommon {
  int sync_interval;  // NTP client's period
  int polling_time;   // Clock Manager thread's period
} PlClockManagerCommon;

typedef struct PlClockManagerSettingSkipAndLimit {
  PlClockManagerParamType type;
  int limit_packet_time;
  int limit_rtc_correction_value;
  int sanity_limit;
} PlClockManagerSkipAndLimit;

typedef struct PlClockManagerSettingSlewParam {
  PlClockManagerParamType type;
  int stable_rtc_correction_value;
  int stable_sync_number;
} PlClockManagerSlewParam;

typedef struct PlClockManagerParams {
  PlClockManagerConnection connect;
  PlClockManagerCommon common;
  PlClockManagerSkipAndLimit skip_and_limit;
  PlClockManagerSlewParam slew_setting;
} PlClockManagerParams;

typedef void *PlClockManagerNtpStatus;

// """Waits for terminating the NTP client daemon.

// This function waits for the NTP client daemon to terminate.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kPlClockManagerSuccess: success

// """
PlClockManagerReturnValue PlClockManagerWaitForTerminatingDaemon(void);

// """Deletes all NTP client configuration files.

// This function removes all NTP client configuration files created by the
// clock manager.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kPlClockManagerSuccess: success
//    kPlClockManagerInternalError: deletion failed

// """
PlClockManagerReturnValue PlClockManagerDeleteConfFiles(void);

// """Gets the current NTP synchronization status.

// This function allocates memory for a status value and queries
// the NTP client tracking status. The returned status pointer can be used
// with PlClockManagerJudgeNtpTimeSyncComplete to determine sync completion.
// The caller is responsible for freeing the returned pointer.

// Args:
//    no arguments.

// Returns:
//    Pointer to NTP status data (bool*), or NULL if failed.
//    The status indicates whether the NTP client is tracking normally.

// """
PlClockManagerNtpStatus PlClockManagerGetNtpStatus(void);

// """Judges whether NTP time synchronization has completed.

// This function examines the NTP status data obtained from
// PlClockManagerGetNtpStatus and determines the synchronization state.
// The status parameter should be a pointer returned by
// PlClockManagerGetNtpStatus.

// Args:
//    status: Pointer to NTP status data (obtained from
//    PlClockManagerGetNtpStatus).
//            Can be NULL.

// Returns:
//    Synchronization status. The following values are returned:
//    kPlClockManagerNtpTimeSyncSuccess:
//      time synchronization completed successfully
//    kPlClockManagerNtpTimeSyncFailure:
//      time synchronization failed (currently not used)
//    kPlClockManagerHasNotCompletedYet:
//      synchronization not completed or status is NULL

// """
PlClockManagerNtpTimeSyncStatus PlClockManagerJudgeNtpTimeSyncComplete(
    PlClockManagerNtpStatus status);

// """Starts the NTP client daemon with specified parameters.

// This function configures and starts the NTP client daemon.
// It sets up server configuration, synchronization intervals, and filtering
// rules based on the provided parameters, then starts the daemon.

// Args:
//    param: Pointer to PlClockManagerParams structure containing:
//           - connect: NTP server connection settings
//           - common: synchronization and polling intervals
//           - skip_and_limit: packet filtering and sanity limit settings
//           - slew_setting: step mode configuration

// Returns:
//    Results. The following value is returned.
//    kPlClockManagerSuccess: success
//    kPlClockManagerParamError: parameter validation failed
//    kPlClockManagerInternalError: configuration setup failed
//    kPlClockManagerStateTransitionError: daemon start failed

// """
PlClockManagerReturnValue PlClockManagerStartNtpClientDaemon(
    const PlClockManagerParams *param);

// """Restarts the NTP client daemon.

// This function restarts the NTP client daemon to apply any configuration
// changes or recover from errors.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kPlClockManagerSuccess: success
//    kPlClockManagerStateTransitionError: restart failed

// """
PlClockManagerReturnValue PlClockManagerRestartNtpClientDaemon(void);

// """Checks if the NTP client daemon is active.

// This function checks whether the NTP client daemon is currently running
// and active.

// Args:
//    no arguments.

// Returns:
//    true if the daemon is active, false otherwise.

// """
bool PlClockManagerIsNtpClientDaemonActive(void);

/**
 * @brief Get migration data from system configuration
 *
 * This function retrieves migration data from system configuration files.
 * Currently implemented to read NTP server information from /etc/chrony.conf.
 *
 * @param[out] dst Destination buffer to store the migration data
 * @param[in] dst_size Size of the destination buffer
 *
 * @return PlClockManagerReturnValue
 * @retval kPlClockManagerSuccess Success
 * @retval kPlClockManagerParamError Invalid parameters
 * @retval kPlClockManagerInternalError Failed to access configuration file or
 * no data found
 */
PlClockManagerReturnValue PlClockManagerGetMigrationDataImpl(void *dst,
                                                              size_t dst_size);

/**
 * @brief Setup migration configuration for clock management
 * 
 * This function performs the complete setup for clock management migration,
 * including the following operations:
 * 1. Adds "confdir /etc/chrony/conf.d" to the end of chrony.conf if not present
 * 2. Replaces default NTP server with Aitrios NTP server (time.aitrios.sony-semicon.com)
 * 3. Creates symbolic link from /misc/smartcamera/edc/conf.d to /etc/chrony/conf.d
 * 
 * @return PlClockManagerReturnValue
 * @retval kPlClockManagerSuccess Success
 * @retval kPlClockManagerInternalError Failed to modify configuration or create symbolic link
 */
PlClockManagerReturnValue PlClockManagerSetupMigration(void);

#endif  // PL_CLOCK_MANAGER_H_
