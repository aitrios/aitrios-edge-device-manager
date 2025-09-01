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
// In the current implementation, it immediately returns success
// as the daemon termination is handled synchronously.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kPlClockManagerSuccess: success

// """
PlClockManagerReturnValue PlClockManagerWaitForTerminatingDaemon(void);

// """Deletes all NTP client configuration files.

// This function removes all chronyd configuration files created by the
// clock manager. It calls the underlying chrony implementation to delete
// server.conf, maxchange.conf, and makestep.conf files.

// Args:
//    no arguments.

// Returns:
//    Results. The following value is returned.
//    kPlClockManagerSuccess: success
//    kPlClockManagerInternalError: deletion failed

// """
PlClockManagerReturnValue PlClockManagerDeleteConfFiles(void);

// """Gets the current NTP synchronization status.

// This function allocates memory for a boolean status value and queries
// the chronyd tracking status. The returned status pointer can be used
// with PlClockManagerJudgeNtpTimeSyncComplete to determine sync completion.
// The caller is responsible for freeing the returned pointer.

// Args:
//    no arguments.

// Returns:
//    Pointer to NTP status data (bool*), or NULL if failed.
//    The status indicates whether chronyd is tracking normally.

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
//    kNtpTimeSyncSuccess: time synchronization completed successfully
//    kNtpTimeSyncFailure: time synchronization failed (currently not used)
//    kHasNotCompletedYet: synchronization not completed or status is NULL

// """
PlClockManagerNtpTimeSyncStatus PlClockManagerJudgeNtpTimeSyncComplete(
    PlClockManagerNtpStatus status);

// """Starts the NTP client daemon with specified parameters.

// This function configures and starts the chronyd NTP client daemon.
// It creates the configuration directory, sets up server configuration,
// maximum change limits, and step mode settings based on the provided
// parameters, then restarts the chronyd service to apply the new configuration.

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
//    kPlClockManagerStateTransitionError: chronyd restart failed

// """
PlClockManagerReturnValue PlClockManagerStartNtpClientDaemon(
    const PlClockManagerParams *param);

PlClockManagerReturnValue PlClockManagerRestartNtpClientDaemon(void);

bool PlClockManagerIsNtpClientDaemonActive(void);

#endif  // PL_CLOCK_MANAGER_H_
