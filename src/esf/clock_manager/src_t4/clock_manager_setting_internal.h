/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_SETTING_INTERNAL_H_
#define ESF_CLOCK_MANAGER_SETTING_INTERNAL_H_

#include <stdbool.h>

#include "clock_manager.h"
#include "clock_manager_setting.h"
#include "clock_manager_utility.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Declarations by typedef
 */

typedef uint32_t EsfClockManagerPsmParamsType;

/**
 * Definitions of structures
 */

// This structure is for Parameter Storage Manager's functions.
// This structure represents either a host name or an IPv4 address for an NTP
// server.
// The following is examples:
// In case host name; "ntp.nict.jp".
// In case IPv4 address; "192.168.1.100".
typedef struct EsfClockManagerSettingConnectionForPsm {
  char hostname[ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE];
} EsfClockManagerConnectionForPsm;

// This structure is for Parameter Storage Manager's functions.
// This structure represents which members in object of struct
// EsfClockManagerSettingConnection turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingConnectionForPsmMask {
  uint8_t hostname : 1;
} EsfClockManagerConnectionForPsmMask;

// This is a structure for Parameter Storage Manager to handle the variable
// sync_interval as raw-data type.
typedef struct EsfParameterStorageManagerRawSyncInterval {
  uint32_t size;
  int sync_interval;  // NTP client's period
} EsfParameterStorageManagerRawSyncInterval;

// This is a structure for Parameter Storage Manager to handle the variable
// polling_time as raw-data type.
typedef struct EsfParameterStorageManagerRawPollingTime {
  uint32_t size;
  int polling_time;  // Clock Manager thread's period
} EsfParameterStorageManagerRawPollingTime;

// This structure is for Parameter Storage Manager's functions.
// This structure represents a period which the NTP client sends a message to
// the NTP server for, and a period which a thread of Clock Manager keeps the
// NTP client under surveillance.
typedef struct EsfClockManagerSettingCommonForPsm {
  EsfParameterStorageManagerRawSyncInterval
      raw_sync_interval;  // NTP client's period
  EsfParameterStorageManagerRawPollingTime
      raw_polling_time;  // Clock Manager thread's period
} EsfClockManagerCommonForPsm;

// This structure is for Parameter Storage Manager's functions.
// This structure represents which members in object of struct
// EsfClockManagerSettingCommon turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingCommonForPsmMask {
  uint8_t raw_sync_interval : 1;  // NTP client's period
  uint8_t raw_polling_time : 1;   // Clock Manager thread's period
} EsfClockManagerCommonForPsmMask;

// This is a structure for Parameter Storage Manager to handle the variable
// type as raw-data type.
typedef struct EsfParameterStorageManagerRawType {
  uint32_t size;
  EsfClockManagerPsmParamsType type;
} EsfParameterStorageManagerRawType;

// This is a structure for Parameter Storage Manager to handle the variable
// limit_packet_time as raw-data type.
typedef struct EsfParameterStorageManagerRawLimitPacketTime {
  uint32_t size;
  int limit_packet_time;
} EsfParameterStorageManagerRawLimitPacketTime;

// This is a structure for Parameter Storage Manager to handle the variable
// limit_rtc_correction_value as raw-data type.
typedef struct EsfParameterStorageManagerRawLimitRtcCorrectionValue {
  uint32_t size;
  int limit_rtc_correction_value;
} EsfParameterStorageManagerRawLimitRtcCorrectionValue;

// This is a structure for Parameter Storage Manager to handle the variable
// sanity_limit as raw-data type.
typedef struct EsfParameterStorageManagerRawSanityLimit {
  uint32_t size;
  int sanity_limit;
} EsfParameterStorageManagerRawSanityLimit;

// This structure is for Parameter Storage Manager's functions.
// This structure represents the followings:
// - raw_type: shows that member variables except this type are used which
//   value.
// - raw_limit_packet_time: If the absolute value of delta of sample equals or
//   less than this value, the sample is used as an NTP sampling, where delta
//   is defined in RFC 5905; delta implies round-trip delay.
// - raw_limit_rtc_correction_value: If the absolute value of theta of a sample
//   is greater than this value, the theta of the sample is changed to
//   ``sgn(theta) * (this value)'', where theta is defined in RFC 5905; theta
//   implies offset.
// - raw_sanity_limit: If the absolute value of theta of a sample is greater
//   than this value, the sample is regarded as a singularity, where theta is
//   defined in RFC 5905; theta implies offset.
typedef struct EsfClockManagerSettingSkipAndLimitForPsm {
  EsfParameterStorageManagerRawType raw_type;
  EsfParameterStorageManagerRawLimitPacketTime raw_limit_packet_time;
  EsfParameterStorageManagerRawLimitRtcCorrectionValue
      raw_limit_rtc_correction_value;
  EsfParameterStorageManagerRawSanityLimit raw_sanity_limit;
} EsfClockManagerSkipAndLimitForPsm;

// This structure is for Parameter Storage Manager's functions.
// This structure represents which members in object of struct
// EsfClockManagerSettingSkipAndLimitMask turn on.
// A member variable is one implies that it turns on, and the member variable
// is zero implies that it turns off.
typedef struct EsfClockManagerSettingSkipAndLimitForPsmMask {
  uint8_t raw_type : 1;
  uint8_t raw_limit_packet_time : 1;
  uint8_t raw_limit_rtc_correction_value : 1;
  uint8_t raw_sanity_limit : 1;
} EsfClockManagerSkipAndLimitForPsmMask;

// This is a structure for Parameter Storage Manager to handle the variable
// stable_rtc_correction_value as raw-data type.
typedef struct EsfParameterStorageManagerRawStableRtcCorrectionValue {
  uint32_t size;
  int stable_rtc_correction_value;
} EsfParameterStorageManagerRawStableRtcCorrectionValue;

// This is a structure for Parameter Storage Manager to handle the variable
// stable_sync_number as raw-data type.
typedef struct EsfParameterStorageManagerRawStableSyncNumber {
  uint32_t size;
  int stable_sync_number;
} EsfParameterStorageManagerRawStableSyncNumber;

// This structure is for Parameter Storage Manager's functions.
// This structure represents the followings:
// - raw_type: shows that member variables except this type are used which
//   value.
// - raw_stable_rtc_correction_value: This value is a default interval value
//   which NTP client sends time synchronization messages for.
// - raw_stable_sync_number: This value is a threshold of the number of counts
//   to extend the interval when it happens continuously that absolute value of
//   theta equals or less than an expected time value, in samplings when NTP
//   client sends time synchronization messages in the interval.
typedef struct EsfClockManagerSettingSlewParamForPsm {
  EsfParameterStorageManagerRawType raw_type;
  EsfParameterStorageManagerRawStableRtcCorrectionValue
      raw_stable_rtc_correction_value;
  EsfParameterStorageManagerRawStableSyncNumber raw_stable_sync_number;
} EsfClockManagerSlewParamForPsm;

// This structure is for Parameter Storage Manager's functions.
// This structure represents which members in object of struct
// EsfClockManagerSettingSlewParam turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingSlewParamForPsmMask {
  uint8_t raw_type : 1;
  uint8_t raw_stable_rtc_correction_value : 1;
  uint8_t raw_stable_sync_number : 1;
} EsfClockManagerSlewParamForPsmMask;

// This structure is for Parameter Storage Manager's functions.
// This structure represents a period which a thread of Clock Manager keeps NTP
// client daemon under surveillance, and parameters which pass to NTP client.
typedef struct EsfClockManagerParamsForPsm {
  EsfClockManagerConnectionForPsm connect;
  EsfClockManagerCommonForPsm common;
  EsfClockManagerSkipAndLimitForPsm skip_and_limit;
  EsfClockManagerSlewParamForPsm slew_setting;
} EsfClockManagerParamsForPsm;

// This structure is for Parameter Storage Manager's functions.
// This structure represents which members in object of struct
// EsfClockManagerSetting turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerParamsForPsmMask {
  EsfClockManagerConnectionForPsmMask connect;
  EsfClockManagerCommonForPsmMask common;
  EsfClockManagerSkipAndLimitForPsmMask skip_and_limit;
  EsfClockManagerSlewParamForPsmMask slew_setting;
} EsfClockManagerParamsForPsmMask;

typedef struct EsfClockManagerCondForRw {
  EsfClockManagerCondBase m_cond_base;
  int m_is_in_rw;  // If negative, its absolute is the number of writing
                   // threads; If positive, it is the number of reading
                   // threads.
} EsfClockManagerCondForRw;

typedef struct EsfClockManagerBarrierToPreventParamsFromWriting {
  EsfClockManagerMutexBase m_mutex_base;
  bool m_is_process_in_of_ntp_time_synchronization;
} EsfClockManagerBarrierToPreventParamsFromWriting;

// """Creates and initializes objects regarding with Parameter Storage Manager.

// This function creates objects regarding with Parameter Storage Manager, then
// initializes them.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerInitSetting(void);

// """Deinitializes and frees objects regarding with Parameter Storage Manager.

// This function deinitializes objects regarding with Parameter Storage Manager,
// then frees them.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerDeinitSetting(void);

// """Marks that the process is in of NTP time synchronization.

// This function marks that the process is in of NTP time synchronization.
// initializes them.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerMarkStartingSync(void);

// """Checks whether process is in of NTP time synchronization or not.

// This function marks that the process is in of NTP time synchronization.
// initializes them.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    true: process is in of NTP time synchronization.
//    false: otherwise.

// """
bool EsfClockManagerIsInSync(void);

// """Marks reading the parameters if possible.

// This function marks flag regarding with reading the parameters if there
// exists no thread which is writing parameters.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    true: process is in of NTP time synchronization.
//    false: otherwise.

// """
bool EsfClockManagerEnterReadingIfPossible(void);

// """Marks writing the parameters if possible.

// This function marks flag regarding with writing the parameters if the status
// is out of NTP time synchronization-process and there exists no thread which
// is writing/reading parameters.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    true: process is in of NTP time synchronization.
//    false: otherwise.

// """
bool EsfClockManagerEnterWritingIfPossible(void);

// """Marks leaving from reading the parameters.

// This function decreases the counter regarding with reading the parameters.

// Args:
//    no arguments.

// Returns:
//    void

// """
void EsfClockManagerLeaveReading(void);

// """Marks leaving from writing the parameters.

// This function increases the counter regarding with writing the parameters.

// Args:
//    no arguments.

// Returns:
//    void

// """
void EsfClockManagerLeaveWriting(void);

// """Marks that the process is out of NTP time synchronization-process.

// This function marks that the process is out of NTP time
// synchronization-process.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerMarkCompletedSync(void);

EsfClockManagerReturnValue EsfClockManagerSetParamsForciblyMain(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask);

EsfClockManagerReturnValue EsfClockManagerSetParamsMain(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask);

EsfClockManagerReturnValue EsfClockManagerGetParamsMain(
    EsfClockManagerParams *const data);

// """Gets a copy object of the object in volatile memory.

// This function returns a copy of the object which g_params_in_volatile points.

// Args:
//    params_obj (EsfClockManagerParams *const): a pointer to an object of
//      EsfClockManagerParams.  An object that points this pointer is
//      substituted for a copy of the object which g_params_in_volatile points.
//    tag (const int): an integer which indicates the caller.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerGetParamsInternal(
    EsfClockManagerParams *const params_obj, const int tag);

// """Stores parameters on volatile memory in non-volatile via P.S.M.

// This function returns a copy of the object which g_params_in_volatile points.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerSaveParamsInternal(void);

// """Registers a callback function called when factory reset.

// This function registers a callback function called when factory reset.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter error.
//    kClockManagerInternalError: internal error broke out.

// """
EsfClockManagerReturnValue EsfClockManagerRegisterFactoryResetCb(void);

// """Unregisters a callback function called when factory reset.

// This function unregisters a callback function called when factory reset.

// Args:
//    no arguments.

// Returns:
//    The following values are returned:
//    kClockManagerSuccess: success.

// """
EsfClockManagerReturnValue EsfClockManagerUnregisterFactoryResetCb(void);
#ifdef __cplusplus
}
#endif

#endif  // ESF_CLOCK_MANAGER_SETTING_INTERNAL_H_
