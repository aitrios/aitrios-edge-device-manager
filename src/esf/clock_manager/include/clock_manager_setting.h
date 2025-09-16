/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_SETTING_H_
#define ESF_CLOCK_MANAGER_SETTING_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __NuttX__
#include <nuttx/config.h>
#endif

/**
 * Definitions of macros
 */

#define CLOCK_MANAGER_DEFAULT_NTP_SERVER \
  CONFIG_EXTERNAL_CLOCK_MANAGER_DEFAULT_NTP_SERVER

#define CLOCK_MANAGER_SYNC_INTERVAL_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_SYNC_INTERVAL_MIN)
#define CLOCK_MANAGER_SYNC_INTERVAL_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_SYNC_INTERVAL_MAX)
#define CLOCK_MANAGER_SYNC_INTERVAL_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_SYNC_INTERVAL_DEF)

#define CLOCK_MANAGER_POLLING_TIME_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_POLLING_TIME_MIN)
#define CLOCK_MANAGER_POLLING_TIME_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_POLLING_TIME_MAX)
#define CLOCK_MANAGER_POLLING_TIME_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_POLLING_TIME_DEF)

#define CLOCK_MANAGER_LIMIT_PACKET_TIME_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_LIMIT_PACKET_TIME_MIN)
#define CLOCK_MANAGER_LIMIT_PACKET_TIME_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_LIMIT_PACKET_TIME_MAX)
#define CLOCK_MANAGER_LIMIT_PACKET_TIME_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_LIMIT_PACKET_TIME_DEF)

#define CLOCK_MANAGER_RTC_CORRECT_LIMIT_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_RTC_CORRECT_LIMIT_MIN)
#define CLOCK_MANAGER_RTC_CORRECT_LIMIT_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_RTC_CORRECT_LIMIT_MAX)
#define CLOCK_MANAGER_RTC_CORRECT_LIMIT_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_RTC_CORRECT_LIMIT_DEF)

#define CLOCK_MANAGER_SANITY_LIMIT_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_SANITY_LIMIT_MIN)
#define CLOCK_MANAGER_SANITY_LIMIT_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_SANITY_LIMIT_MAX)
#define CLOCK_MANAGER_SANITY_LIMIT_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_SANITY_LIMIT_DEF)

#define CLOCK_MANAGER_STABLE_RTC_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_RTC_MIN)
#define CLOCK_MANAGER_STABLE_RTC_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_RTC_MAX)
#define CLOCK_MANAGER_STABLE_RTC_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_RTC_DEF)

#define CLOCK_MANAGER_STABLE_SYNC_CONT_MIN \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_SYNC_CONT_MIN)
#define CLOCK_MANAGER_STABLE_SYNC_CONT_MAX \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_SYNC_CONT_MAX)
#define CLOCK_MANAGER_STABLE_SYNC_CONT_DEF \
  (CONFIG_EXTERNAL_CLOCK_MANAGER_STABLE_SYNC_CONT_DEF)

#include <limits.h>
#include <stdint.h>

#include "clock_manager.h"
#include "parameter_storage_manager_common.h"

/**
 * Definitions of macros
 */

// The maximum size of a host name or an IPv4 address for an NTP server.
// This size includes a terminal null character (i.e., '\0').
#define ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE (272)

/**
 * Definitions of enumerations
 */

typedef enum EsfClockManagerParamType {
  kClockManagerParamTypeOff,
  kClockManagerParamTypeDefault,
  kClockManagerParamTypeCustom,
  kClockManagerParamTypeNumMax
} EsfClockManagerParamType;

/**
 * Definitions of structures
 */

// This structure represents either a host name or an IPv4 address for an NTP
// server.
// The following is examples:
// In case host name; "ntp.nict.jp".
// In case IPv4 address; "192.168.1.100".
typedef struct EsfClockManagerSettingConnection {
  char hostname[ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE];
} EsfClockManagerConnection;

// This structure represents which members in object of struct
// EsfClockManagerSettingConnection turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingConnectionMask {
  uint8_t hostname : 1;
} EsfClockManagerConnectionMask;

// This structure represents a period which the NTP client sends a message to
// the NTP server for, and a period which a thread of Clock Manager keeps the
// NTP client under surveillance.
typedef struct EsfClockManagerSettingCommon {
  int sync_interval;  // NTP client's period
  int polling_time;   // Clock Manager thread's period
} EsfClockManagerCommon;

// This structure represents which members in object of struct
// EsfClockManagerSettingCommon turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingCommonMask {
  uint8_t sync_interval : 1;
  uint8_t polling_time : 1;
} EsfClockManagerCommonMask;

// This structure represents the followings:
// - type: shows that member variables except this type are used which value.
// - limit_packet_time: If the absolute value of delta of sample equals or less
//   than this value, the sample is used as an NTP sampling, where delta is
//   defined in RFC 5905; delta implies round-trip delay.
// - limit_rtc_correction_value: If the absolute value of theta of a sample is
//   greater than this value, the theta of the sample is changed to
//   ``sgn(theta) * (this value)'', where theta is defined in RFC 5905; theta
//   implies offset.
// - sanity_limit: If the absolute value of theta of a sample is greater than
//   this value, the sample is regarded as a singularity, where theta is defined
//   in RFC 5905; theta implies offset.
typedef struct EsfClockManagerSettingSkipAndLimit {
  EsfClockManagerParamType type;
  int limit_packet_time;
  int limit_rtc_correction_value;
  int sanity_limit;
} EsfClockManagerSkipAndLimit;

// This structure represents which members in object of struct
// EsfClockManagerSettingSkipAndLimitMask turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingSkipAndLimitMask {
  uint8_t type : 1;
  uint8_t limit_packet_time : 1;
  uint8_t limit_rtc_correction_value : 1;
  uint8_t sanity_limit : 1;
} EsfClockManagerSkipAndLimitMask;

// This structure represents the followings:
// - type: shows that member variables except this type are used which value.
// - stable_rtc_correction_value: This value is a default interval value which
//   NTP client sends time synchronization messages for.
// - stable_sync_number: This value is a threshold of the number of counts to
//   extend the interval when it happens continuously that absolute value of
//   theta equals or less than an expected time value, in samplings when NTP
//   client sends time synchronization messages in the interval.
typedef struct EsfClockManagerSettingSlewParam {
  EsfClockManagerParamType type;
  int stable_rtc_correction_value;
  int stable_sync_number;
} EsfClockManagerSlewParam;

// This structure represents which members in object of struct
// EsfClockManagerSettingSlewParam turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerSettingSlewParamMask {
  uint8_t type : 1;
  uint8_t stable_rtc_correction_value : 1;
  uint8_t stable_sync_number : 1;
} EsfClockManagerSlewParamMask;

// This structure represents a period which a thread of Clock Manager keeps NTP
// client daemon under surveillance, and parameters which pass to NTP client.
typedef struct EsfClockManagerParams {
  EsfClockManagerConnection connect;
  EsfClockManagerCommon common;
  EsfClockManagerSkipAndLimit skip_and_limit;
  EsfClockManagerSlewParam slew_setting;
} EsfClockManagerParams;

// This structure represents which members in object of struct
// EsfClockManagerSetting turn on.
// A member variable is one implies that it turns on, and the member variable is
// zero implies that it turns off.
typedef struct EsfClockManagerParamsMask {
  EsfClockManagerConnectionMask connect;
  EsfClockManagerCommonMask common;
  EsfClockManagerSkipAndLimitMask skip_and_limit;
  EsfClockManagerSlewParamMask slew_setting;
} EsfClockManagerParamsMask;

/**
 * Declarations of public functions
 */

// """Saves data in non-volatile memory via Parameter Storage Manager.

// The data given by this function is saved in non-volatile memory via Parameter
// Storage Manager, if the member of the mask given by this function turns on,
// where the member is corresponding to data.

// Args:
//    data (const EsfClockManagerParams *): a pointer to an object of
//      EsfClockManagerParams which you want to save.
//    mask (const EsfClockManagerParamsMask *): a pointer to an object that
//      represents which member variables of EsfClockManagerParams are that you
//      want to save.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter(s).
//    kClockManagerInternalError:
//    kClockManagerStateTransitionError:

// """
EsfClockManagerReturnValue EsfClockManagerSetParamsForcibly(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask);

// """Saves data in volatile memory

// The data given by this function is saved in volatile memory,
// if the member of the mask given by this function turns on, where
// the member is corresponding to data.
// If NTP synchronization which is started by it that EsfClockManagerStart is
// called is complete in success, the parameters in volatile memory are written
// in non-volatile.  Otherwise, the parameters in volatile memory are
// overwritten by values of parameters in non-volatile memory.

// Args:
//    data (const EsfClockManagerParams *): a pointer to an object of
//      EsfClockManagerParams which you want to save.
//    mask (const EsfClockManagerParamsMask *): a pointer to an object that
//      represents which member variables of EsfClockManagerParams are that you
//      want to save.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter(s).
//    kClockManagerInternalError:
//    kClockManagerStateTransitionError:

// """
EsfClockManagerReturnValue EsfClockManagerSetParams(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask);

// """Reads data from volatile/non-volatile memory.

// Data which is read from volatile memory is substituted for data pointer given
// by this function.
// If there does not exist data in volatile memory, then returns data which is
// got from non-volatile memory via Parameter Storage Manager.

// Args:
//    data (EsfClockManagerParams *const): a pointer to an object of
//      EsfClockManagerParams which saves data which you want to
//      read.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success.
//    kClockManagerParamError: invalid parameter(s).
//    kClockManagerInternalError:
//    kClockManagerStateTransitionError:

// """
EsfClockManagerReturnValue EsfClockManagerGetParams(
    EsfClockManagerParams *const data);

/**
 * @brief Migrate NTP URL configuration data
 *
 * This function migrates NTP URL configuration data from the old format
 * to the new format during system upgrades. It retrieves migration data
 * using the platform layer implementation.
 *
 * @param[out] dst Destination buffer to store migrated data
 * @param[in] dst_size Size of the destination buffer
 *
 * @return EsfClockManagerReturnValue
 * @retval kClockManagerSuccess Migration completed successfully
 * @retval kClockManagerInternalError Failed to retrieve migration data
 *
 * @note This function is specific to T4 platform implementation
 */
EsfClockManagerReturnValue EsfClockManagerGetMigrationData(void *dst,
                                                           size_t dst_size);

/**
 * @brief Perform clock manager settings migration
 *
 * This function performs migration of clock manager settings by retrieving
 * NTP URL from migration data and updating the current configuration.
 * It first gets the migration data using EsfClockManagerGetMigrationData,
 * then retrieves current parameters using EsfClockManagerGetParams.
 *
 * @return EsfClockManagerReturnValue
 * @retval kClockManagerSuccess Migration completed successfully
 * @retval kClockManagerParamError Invalid parameter error
 * @retval kClockManagerInternalError Failed to retrieve migration data or
 * current parameters
 *
 * @note This function is specific to T4 platform implementation
 */
EsfClockManagerReturnValue EsfClockManagerMigrateSettings(void);

#ifdef __cplusplus
}
#endif

#endif  // ESF_CLOCK_MANAGER_SETTING_H_
