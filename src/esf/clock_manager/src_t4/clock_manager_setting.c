/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager_setting.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clock_manager_internal.h"
#include "clock_manager_setting_internal.h"
#include "clock_manager_utility.h"
#include "parameter_storage_manager.h"
#include "parameter_storage_manager_common.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

/**
 * Declarations of private functions --- i.e., functions given storage-class
 * specifier static.
 */

// """Frees resources.

// This function frees resources.

// Args:
//    no arguments.

// Returns:
//    void

// """
STATIC void EsfClockManagerFreeResource(void);

// """Checks the parameters regarding with a connection for validity.

// This function checks if the given connection parameter is valid. It verifies
// that the connection object is not NULL and that the hostname string does not
// exceed the maximum size.

// Args:
//    connect (const EsfClockManagerConnection *const): the connection object
//      to be checked.
//    mask_connect (const EsfClockManagerConnectionMask *const): a pointer to an
//      object that represents which member variables of
//      EsfClockManagerConnection are that you want to check.

// Returns:
//    Returns true if the connection parameters are valid, false otherwise.

// """
STATIC bool EsfClockManagerCheckParamConnect(
    const EsfClockManagerConnection *const connect,
    const EsfClockManagerConnectionMask *const mask_connect);

// """Check if the provided EsfClockManagerCommon structure is valid.

// This function checks if the provided EsfClockManagerCommon structure is valid
// by verifying the following:
// - The structure pointer is not NULL.
// - The sync_interval value is within the valid range of
// CLOCK_MANAGER_SYNC_INTERVAL_MIN and CLOCK_MANAGER_SYNC_INTERVAL_MAX.
// - The polling_time value is within the valid range of
// CLOCK_MANAGER_POLLING_TIME_MIN and CLOCK_MANAGER_POLLING_TIME_MAX.
//

// Args:
//    common (const EsfClockManagerCommon * const): A pointer to the
//      EsfClockManagerCommon structure to be checked.
//    mask_common (const EsfClockManagerCommonMask *const):

// Returns:
//    Returns true if the structure is valid, false otherwise.

// """
STATIC bool EsfClockManagerCheckParamCommon(
    const EsfClockManagerCommon *const common,
    const EsfClockManagerCommonMask *const mask_common);

// """Checks the validity of the parameters in the structure.

// EsfClockManagerSkipAndLimit structure.
//
// This function checks if the provided EsfClockManagerSkipAndLimit structure is
// valid by verifying the range of its members. The function returns true if all
// the parameters are within the valid range, otherwise it returns false.
//

// Args:
//    skip_and_limit (const EsfClockManagerSkipAndLimit * const): A pointer to
//      the EsfClockManagerSkipAndLimit structure to be checked.
//    mask_skip_and_limit (const EsfClockManagerSkipAndLimitMask *const): A
//      pointer to an object that represents which member variables of
//      EsfClockManagerSkipAndLimit are that you want to check.

// Returns:
//    Returns true if all the parameters are within the valid range,
//    otherwise returns false.

// """
STATIC bool EsfClockManagerCheckParamSkipAndLimit(
    const EsfClockManagerSkipAndLimit *const skip_and_limit,
    const EsfClockManagerSkipAndLimitMask *const mask_skip_and_limit);

// """Check the validity of the EsfClockManagerSlewParam structure.

// This function checks if the provided EsfClockManagerSlewParam structure is
// valid. It verifies that the structure is not NULL, and that the values of its
// members are within the specified range.

// Args:
//    slew_setting (const EsfClockManagerSlewParam * const): A pointer to the
//      EsfClockManagerSlewParam structure to be checked.
//    mask_slew_setting (const EsfClockManagerSlewParamMask *const): A pointer
//      to an object that represents which member variables of
//      EsfClockManagerSkipAndLimit are that you want to check.

// Returns:
//    Returns true if the structure is valid, and false otherwise.

// Note:
//    This function assumes that the CLOCK_MANAGER_STABLE_RTC_MIN,
//    CLOCK_MANAGER_STABLE_RTC_MAX, and CLOCK_MANAGER_STABLE_SYNC_CONT_MIN,
//    CLOCK_MANAGER_STABLE_SYNC_CONT_MAX constants are defined.

// """
STATIC bool EsfClockManagerCheckParamSlewParam(
    const EsfClockManagerSlewParam *const slew_setting,
    const EsfClockManagerSlewParamMask *const mask_slew_setting);

// """Verifies parameters.

// This function verifies the given parameters which correspond to members
// whose mask turns on.

// Args:
//    data (const EsfClockManagerParams * const): a pointer to data (i.e.,
//      object of EsfClockManagerParam) which you want to verify.
//    mask (const EsfClockManagerParamsMask * const): a pointer to mask (i.e.,
//      object of EsfClockManagerMask) which represents kinds of data which
//      you want to verify.

// Returns:
//    Results.  The following value is returned.
//    kClockManagerSuccess: success
//    kClockManagerParamError: invalid parameter
//    kClockManagerStateTransitionError: status translation failure

// """
STATIC bool EsfClockManagerCheckParam(
    const EsfClockManagerParams *const data,
    const EsfClockManagerParamsMask *const mask);

// """Verifies whether hostname in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `hostname' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsHostnameMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether sync_interval in common in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `sync_interval' in `common' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsSyncIntervalMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether polling_time in common in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `polling_time' in `common' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsPollingTimeMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether type in skip_and_limit in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `type' in `skip_and_limit' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsTypeMaskInSkipAndLimitEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether limit_packet_time in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `limit_packet_time' in `skip_and_limit' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsLimitPacketTimeMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether limit_rtc_correction_value in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `limit_rtc_correction_value' in `skip_and_limit' in the given pointer to an
// object of EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsLimitRtcCorrectionValueMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether sanity_limit in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `sanity_limit' in `skip_and_limit' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsSanityLimitMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether type in slew_setting in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `type' in `slew_setting' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsTypeMaskInSlewParamEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether stable_rtc_correction_value in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `stable_rtc_correction_value' in `slew_setting' in the given pointer to an
// object of EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsStableRtcCorrectionValueMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Verifies whether stable_sync_number in the given mask turns on.

// This function returns a value of bool corresponding to the member variable
// `stable_sync_number' in `slew_setting' in the given pointer to an object of
// EsfClockManagerParamsMask.

// Args:
//    mask (EsfParameterStorageManagerMask): a pointer to an object of
//      EsfClockManagerParamsMask.

// Returns:
//    true: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 1.
//    false: if the member variable in the given pointer to an object of
//      EsfClockManagerParamsMask is 0.
//

// """
STATIC bool EsfClockManagerIsStableSyncNumberMaskEnabled(
    EsfParameterStorageManagerMask mask);

// """Is called when factory reset.

// If this function is registered in Parameter Storage Manager, this function is
// called when factory reset.

// Args:
//    private_data (void *): a pointer to private date

// Returns:
//    Value of EsfParameterStorageManagerStatus.
//

// """
STATIC EsfParameterStorageManagerStatus
EsfClockManagerOnFactoryReset(void *private_data);

// """Loads the parameters from Parameter Storage Manager.

// This function loads the parameters from Parameter Storage Manager.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an object which
//      each value that is read from Parameter Storage Manager is store in.
//    tag (const int): an integer which indicates the caller.

// Returns:
//    Value of EsfParameterStorageManagerStatus.
//

// """
STATIC EsfClockManagerReturnValue EsfClockManagerLoadFromPsm(
    EsfClockManagerParamsForPsm *const psm_obj, const int tag);

// """Checks then substitutes the default value for connect-member if necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an object
//    whose each value are desired validating.

// Returns:
//    void
//

// """
STATIC void EsfClockManagerCheckPsmConnectThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Checks then substitutes the default value for common-member if necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an object whose
//    each value are desired validating.

// Returns:
//    void
//

// """
STATIC void EsfClockManagerCheckPsmCommonThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Checks then substitutes the default value for sub-skip-limit-member if
// necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an
//    object whose each value are desired validating.

// Returns:
//    void
//

// """
STATIC void
EsfClockManagerCheckPsmSub1SkipAndLimitThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Checks then substitutes the default value for sub-skip-limit-member if
// necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an
//    object whose each value are desired validating.

// Returns:
//    void
//

// """
STATIC void
EsfClockManagerCheckPsmSub2SkipAndLimitThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Checks then substitutes the default value for skip-limit-member if
// necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an
//    object whose each value are desired validating.

// Returns:
//    void
//

// """
STATIC void
EsfClockManagerCheckPsmSkipAndLimitThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Checks then substitutes the default value for slew-member if necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an
//    object whose each value are desired validating.

// Returns:
//    void
//

// """
STATIC void
EsfClockManagerCheckPsmSlewSettingThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Checks then substitutes the default values for the members if necessary.

// This function validates each value of the given object's member variable.
// If value is not valid, the default value is substituted for it.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an object whose
//    each value are desired validating.

// Returns:
//    void
//

// """
STATIC void EsfClockManagerCheckPsmDataThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj);

// """Gets default value corresponds with given item id.

// This function substitutes the member variable the given psm_obj for default
// value of item indicated by item_id.

// Args:
//    psm_obj (EsfClockManagerParamsForPsm *const): a pointer to an object whose
//      each value are desired validating.
//    item_id (EsfParameterStorageManagerItemID): item id.

// Returns:
//    void
//

// """
STATIC void EsfClockManagerGetPsmDefault(
    EsfClockManagerParamsForPsm *const psm_obj,
    EsfParameterStorageManagerItemID item_id);

// """Substitutes the given object for the object in volatile.

// The given value of members of EsfClockManagerParams are transformed into a
// value of member of EsfClockManagerParamsForPsm, then the transformed values
// are substituted for the object in volatile memory.
// If value of member variable in mask is 1, and only if, value of member
// variable which has same name in data is substituted for the object in
// volatile memory.

// Args:
//    data (const EsfClockManagerParams *const): a pointer to a variable
//      whose type is EsfClockManagerParams.  Each value of this object's member
//      variables is substituted for member variables of the object in volatile
//      memory.
//    mask (const EsfClockManagerParamsMask *const): a value which you want to
//      store in the object in volatile memory.

// Returns:
//    value of EsfClockManagerReturnValue.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerSubstituteForVolatileObj(
    const EsfClockManagerParams *const data,
    const EsfClockManagerParamsMask *const mask);

// """Transforms the given EsfClockManagerParams to EsfClockManagerParamsForPsm.

// The given value of EsfClockManagerParams is transformed into a value of
// EsfClockManagerParamsForPsm.

// Args:
//    psm_params (EsfClockManagerParamsForPsm *const): a pointer to a variable
//      whose type is EsfClockManagerParamsForPsm.  This is pointer to variable
//      used when a variable which this function's formal parameter params
//      points is transformed into a variable of EsfClockManagerParamsForPsm.
//    params (const EsfClockManagerParams *const): a value which you want to
//      transform into a value of EsfClockManagerParamsForPsm.

// Returns:
//    value of EsfClockManagerReturnValue.

// """
STATIC EsfClockManagerReturnValue
EsfClockManagerParams2PsmParams(EsfClockManagerParamsForPsm *const psm_params,
                                const EsfClockManagerParams *const params);

// """Transforms the given EsfClockManagerParamType to
// EsfClockManagerPsmParamsType.

// Helper function.
// When Parameter Storage Manager uses `type' which corresponds with the member
// variable `type' whose type is EsfClockManagerParamType, its size must be 4
// byte.  Hence this function provides the transform for it.

// Args:
//    type (EsfClockManagerParamType): a value of EsfClockManagerParamType.
//      A value which you want to transform to EsfClockManagerPsmParamsType.

// Returns:
//    value of EsfClockManagerPsmParamsType.

// """
STATIC EsfClockManagerPsmParamsType
EsfClockManagerParamsType2PsmParamsType(EsfClockManagerParamType type);

// """Transforms the given EsfClockManagerParamsForPsm to EsfClockManagerParams.

// The given value of EsfClockManagerParamsForPsm is transformed into a value
// of EsfClockManagerParams.

// Args:
//    params (EsfClockManagerParams *const): a pointer to a variable
//      whose type is EsfClockManagerParams.  This is pointer to variable
//      used when a variable which this function's formal parameter psm_params
//      points is transformed into a variable of EsfClockManagerParams.
//    psm_params (const EsfClockManagerParamsForPsm *const): a value which you
//      want to transform into a value of EsfClockManagerParamsForPsm.

// Returns:
//    value of EsfClockManagerReturnValue.

// """
STATIC EsfClockManagerReturnValue EsfClockManagerPsmParams2Params(
    EsfClockManagerParams *const params,
    const EsfClockManagerParamsForPsm *const psm_params);

/**
 * Definitions of global variables
 */

STATIC const EsfParameterStorageManagerMemberInfo
    kClockManagerParamMembersInfo[] = {
        {
            .id = kEsfParameterStorageManagerItemNTPServer,
            .type = kEsfParameterStorageManagerItemTypeString,
            .offset = offsetof(EsfClockManagerParamsForPsm, connect.hostname),
            .size = ESF_PARAMETER_STORAGE_MANAGER_STRING_SIZEOF(
                EsfClockManagerParamsForPsm, connect.hostname),
            .enabled = EsfClockManagerIsHostnameMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemNTPSyncInterval,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               common.raw_sync_interval),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawSyncInterval, sync_interval),
            .enabled = EsfClockManagerIsSyncIntervalMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemNTPPollingTime,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               common.raw_polling_time),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawPollingTime, polling_time),
            .enabled = EsfClockManagerIsPollingTimeMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemSkipModeSettings,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               skip_and_limit.raw_type),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawType, type),
            .enabled = EsfClockManagerIsTypeMaskInSkipAndLimitEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLimitPacketTime,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               skip_and_limit.raw_limit_packet_time),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawLimitPacketTime,
                limit_packet_time),
            .enabled = EsfClockManagerIsLimitPacketTimeMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemLimitRTCCorrectionValue,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               skip_and_limit.raw_limit_rtc_correction_value),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawLimitRtcCorrectionValue,
                limit_rtc_correction_value),
            .enabled = EsfClockManagerIsLimitRtcCorrectionValueMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemSanityLimit,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               skip_and_limit.raw_sanity_limit),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawSanityLimit, sanity_limit),
            .enabled = EsfClockManagerIsSanityLimitMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemSlewModeSettings,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               slew_setting.raw_type),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawType, type),
            .enabled = EsfClockManagerIsTypeMaskInSlewParamEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStableRTCCorrectionValue,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               slew_setting.raw_stable_rtc_correction_value),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawStableRtcCorrectionValue,
                stable_rtc_correction_value),
            .enabled = EsfClockManagerIsStableRtcCorrectionValueMaskEnabled,
            .custom = NULL,
        },
        {
            .id = kEsfParameterStorageManagerItemStableSyncNumber,
            .type = kEsfParameterStorageManagerItemTypeRaw,
            .offset = offsetof(EsfClockManagerParamsForPsm,
                               slew_setting.raw_stable_sync_number),
            .size = ESF_PARAMETER_STORAGE_MANAGER_RAW_SIZEOF(
                EsfParameterStorageManagerRawStableSyncNumber,
                stable_sync_number),
            .enabled = EsfClockManagerIsStableSyncNumberMaskEnabled,
            .custom = NULL,
        },
};

STATIC const EsfParameterStorageManagerStructInfo kClockManagerParamStructInfo =
    {.items_num = sizeof(kClockManagerParamMembersInfo) /
                  sizeof(kClockManagerParamMembersInfo[0]),
     .items = &kClockManagerParamMembersInfo[0]};

STATIC EsfParameterStorageManagerFactoryResetID
    g_factory_reset_registration_id =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID;

STATIC EsfClockManagerParamsForPsm *g_params_in_volatile = NULL;
STATIC pthread_mutex_t s_params_in_volatile_mutex = PTHREAD_MUTEX_INITIALIZER;

STATIC EsfClockManagerParamsForPsm *g_temporary_for_params = NULL;
STATIC pthread_mutex_t s_temporary_for_params_mutex = PTHREAD_MUTEX_INITIALIZER;

STATIC EsfParameterStorageManagerHandle g_psm_handle =
    ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
STATIC EsfClockManagerCondForRw g_cond_for_rw = {
    .m_cond_base =
        {
            .m_mutex = PTHREAD_MUTEX_INITIALIZER,
            .m_cond = PTHREAD_COND_INITIALIZER,
        },
    .m_is_in_rw = 0,
};
STATIC EsfClockManagerBarrierToPreventParamsFromWriting
    g_barrier_prevent_params_from_writing = {
        .m_mutex_base =
            {
                .m_mutex = PTHREAD_MUTEX_INITIALIZER,
            },
        .m_is_process_in_of_ntp_time_synchronization = false,
};

/**
 * Definitions of public functions
 */

EsfClockManagerReturnValue EsfClockManagerSetParamsForciblyMain(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask) {
  if (g_factory_reset_registration_id ==
          ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID ||
      g_psm_handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  if (EsfClockManagerEnterWritingIfPossible()) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Could not enter the critical region.\n",
                    "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  if (data == NULL || mask == NULL) {
    EsfClockManagerLeaveWriting();
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Any of formal parameters is NULL.\n",
                    "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerParamError;
  }
  if (!EsfClockManagerCheckParam(data, mask)) {
    EsfClockManagerLeaveWriting();
    return kClockManagerParamError;
  }

  EsfClockManagerParamsForPsm *tmp_params = calloc(1, sizeof(*tmp_params));
  if (tmp_params == NULL) {
    EsfClockManagerLeaveWriting();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Memory allocation failed.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  EsfClockManagerParams2PsmParams(tmp_params, data);

  EsfClockManagerCheckPsmConnectThenSubstituteDefaultForIfNecessary(tmp_params);

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      free(tmp_params);
      EsfClockManagerLeaveWriting();
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- Failed to lock mutex:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }

    memcpy(g_temporary_for_params, tmp_params, sizeof(*tmp_params));

    rv_mutex = pthread_mutex_unlock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      free(tmp_params);
      EsfClockManagerLeaveWriting();
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- Failed to unlock mutex:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }
  }

  const EsfParameterStorageManagerStatus saved_status =
      EsfParameterStorageManagerSave(g_psm_handle,
                                     (EsfParameterStorageManagerMask)mask,
                                     (EsfParameterStorageManagerData)tmp_params,
                                     &kClockManagerParamStructInfo, NULL);

  free(tmp_params);
  tmp_params = NULL;

  if (saved_status == kEsfParameterStorageManagerStatusOk) {
    EsfClockManagerSubstituteForVolatileObj(data, mask);
  } else {
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8102);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfParameterStorageManagerSave failed:%d\n",
                     "clock_manager_setting.c", __LINE__, __func__,
                     saved_status);
  }

  const EsfClockManagerReturnValue returned_value =
      saved_status == kEsfParameterStorageManagerStatusOk
          ? kClockManagerSuccess
          : kClockManagerInternalError;

  EsfClockManagerLeaveWriting();
  return returned_value;
}

EsfClockManagerReturnValue EsfClockManagerSetParamsMain(
    const EsfClockManagerParams *data, const EsfClockManagerParamsMask *mask) {
  if (g_factory_reset_registration_id ==
          ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID ||
      g_psm_handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  if (EsfClockManagerEnterWritingIfPossible()) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Could not enter the critical region.\n",
                    "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  if (data == NULL || mask == NULL) {
    EsfClockManagerLeaveWriting();
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Any of formal parameters is NULL.\n",
                    "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerParamError;
  }
  if (!EsfClockManagerCheckParam(data, mask)) {
    EsfClockManagerLeaveWriting();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- member variable of the formal parameter "
                     "is invalid.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerParamError;
  }

  EsfClockManagerSubstituteForVolatileObj(data, mask);

  EsfClockManagerLeaveWriting();

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerGetParamsMain(
    EsfClockManagerParams *const data) {
  if (g_factory_reset_registration_id ==
          ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID ||
      g_psm_handle == ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- EsfClockManagerInit must be called "
                     "before this function is called.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerStateTransitionError;
  }
  if (!EsfClockManagerEnterReadingIfPossible()) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- Could not enter the critical region.\n",
                    "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  if (data == NULL) {
    EsfClockManagerLeaveReading();
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- The formal parameters is NULL.\n",
                    "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerParamError;
  }

  EsfClockManagerGetParamsInternal(data, CALLER_IS_GET_PARAMS);

  EsfClockManagerLeaveReading();

  return kClockManagerSuccess;
}

/**
 * Definitions of package private functions
 */

EsfClockManagerReturnValue EsfClockManagerInitSetting(void) {
  if (g_psm_handle != ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
    WRITE_DLOG_WARN(
        MODULE_ID_SYSTEM, "%s-%d:%s --- PSM has already been opened:%d\n",
        "clock_manager_setting.c", __LINE__, __func__, g_psm_handle);
    return kClockManagerSuccess;
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }

    if (g_params_in_volatile == NULL) {
      g_params_in_volatile = (EsfClockManagerParamsForPsm *)malloc(
          sizeof(EsfClockManagerParamsForPsm));
      if (g_params_in_volatile == NULL) {
        WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                            "clock_manager_setting.c", __LINE__, __func__);
        (void)pthread_mutex_unlock(&s_params_in_volatile_mutex);
        EsfClockManagerFreeResource();
        return kClockManagerInternalError;
      }
    }

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      EsfClockManagerFreeResource();
      return kClockManagerInternalError;
    }
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      EsfClockManagerFreeResource();
      return kClockManagerInternalError;
    }

    if (g_temporary_for_params == NULL) {
      g_temporary_for_params = (EsfClockManagerParamsForPsm *)malloc(
          sizeof(EsfClockManagerParamsForPsm));
      if (g_temporary_for_params == NULL) {
        (void)pthread_mutex_unlock(&s_temporary_for_params_mutex);
        EsfClockManagerFreeResource();
        WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s-%d:%s --- malloc failed.\n",
                            "clock_manager_setting.c", __LINE__, __func__);
        return kClockManagerInternalError;
      }
    }

    rv_mutex = pthread_mutex_unlock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      EsfClockManagerFreeResource();
      return kClockManagerInternalError;
    }
  }

  const EsfParameterStorageManagerStatus psm_status =
      EsfParameterStorageManagerOpen(&g_psm_handle);
  if (psm_status != kEsfParameterStorageManagerStatusOk) {
    EsfClockManagerFreeResource();
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Failed to be open PSM:%d.\n",
                     "clock_manager_setting.c", __LINE__, __func__, psm_status);
    return kClockManagerInternalError;
  }

  g_barrier_prevent_params_from_writing
      .m_is_process_in_of_ntp_time_synchronization = false;

  g_cond_for_rw.m_is_in_rw = 0;

  EsfClockManagerParamsForPsm *tmp_params = calloc(1, sizeof(*tmp_params));
  if (tmp_params == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM, "%s-%d:%s --- calloc failed.\n",
                        "clock_manager_setting.c", __LINE__, __func__);
    EsfClockManagerFreeResource();
    return kClockManagerInternalError;
  }

  tmp_params->connect.hostname[0] = '\0';
  tmp_params->common.raw_sync_interval.sync_interval = -1;
  tmp_params->common.raw_polling_time.polling_time = -1;
  tmp_params->skip_and_limit.raw_type.type =
      (EsfClockManagerPsmParamsType)kClockManagerParamTypeNumMax;
  tmp_params->skip_and_limit.raw_limit_packet_time.limit_packet_time = -1;
  tmp_params->skip_and_limit.raw_limit_rtc_correction_value
      .limit_rtc_correction_value = -1;
  tmp_params->skip_and_limit.raw_sanity_limit.sanity_limit = -1;
  tmp_params->slew_setting.raw_type.type =
      (EsfClockManagerPsmParamsType)kClockManagerParamTypeNumMax;
  tmp_params->slew_setting.raw_stable_rtc_correction_value
      .stable_rtc_correction_value = -1;
  tmp_params->slew_setting.raw_stable_sync_number.stable_sync_number = -1;

  EsfClockManagerReturnValue rv =
      EsfClockManagerLoadFromPsm(tmp_params, CALLER_IS_INIT_SETTING);
  if (rv != kClockManagerSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- Failed to load from PSM:%d.\n",
                     "clock_manager_setting.c", __LINE__, __func__, rv);
    free(tmp_params);
    EsfClockManagerFreeResource();
    return kClockManagerInternalError;
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      free(tmp_params);
      EsfClockManagerFreeResource();
      return kClockManagerInternalError;
    }

    memcpy(g_params_in_volatile, tmp_params,
           sizeof(EsfClockManagerParamsForPsm));
    free(tmp_params);

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      EsfClockManagerFreeResource();
      return kClockManagerInternalError;
    }
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerDeinitSetting(void) {
  EsfClockManagerFreeResource();
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerMarkStartingSync(void) {
  if (pthread_mutex_lock(
          &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  g_barrier_prevent_params_from_writing
      .m_is_process_in_of_ntp_time_synchronization = true;
  pthread_mutex_unlock(
      &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex);

  const int rv_lock = pthread_mutex_lock(&g_cond_for_rw.m_cond_base.m_mutex);
  if (rv_lock) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_setting.c", __LINE__, __func__, rv_lock);
  }
  WRITE_DLOG_INFO(MODULE_ID_SYSTEM, "%s-%d:%s --- ref counter:%d\n",
                  "clock_manager_setting.c", __LINE__, __func__,
                  g_cond_for_rw.m_is_in_rw);

  while (g_cond_for_rw.m_is_in_rw < 0) {
    struct timespec abs_time = {0};
    bool calc_time = EsfClockManagerCalculateAbstimeInMonotonic(&abs_time,
                                                                1000);

    if (!rv_lock && calc_time) {
      pthread_cond_timedwait(&g_cond_for_rw.m_cond_base.m_cond,
                             &g_cond_for_rw.m_cond_base.m_mutex, &abs_time);
    } else {
      sleep((unsigned int)1);
    }
  }

  if (!rv_lock) {
    pthread_mutex_unlock(&g_cond_for_rw.m_cond_base.m_mutex);
  }

  return kClockManagerSuccess;
}

bool EsfClockManagerIsInSync(void) {
  const int rv_lock = pthread_mutex_lock(
      &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex);
  if (rv_lock) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_setting.c", __LINE__, __func__, rv_lock);
  }
  const bool is_process_in_of_sync =
      g_barrier_prevent_params_from_writing
          .m_is_process_in_of_ntp_time_synchronization;
  if (!rv_lock) {
    pthread_mutex_unlock(
        &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex);
  }

  return is_process_in_of_sync;
}

bool EsfClockManagerEnterReadingIfPossible(void) {
  const int rv_lock = pthread_mutex_lock(&g_cond_for_rw.m_cond_base.m_mutex);
  if (rv_lock) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_setting.c", __LINE__, __func__, rv_lock);
  }
  if (g_cond_for_rw.m_is_in_rw >= 0) {
    g_cond_for_rw.m_is_in_rw++;
  }
  const int is_status_in_of_rw = g_cond_for_rw.m_is_in_rw;
  if (!rv_lock) {
    pthread_mutex_unlock(&g_cond_for_rw.m_cond_base.m_mutex);
  }

  return is_status_in_of_rw > 0;
}

bool EsfClockManagerEnterWritingIfPossible(void) {
  const int rv_lock = pthread_mutex_lock(
      &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex);
  if (rv_lock) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_setting.c", __LINE__, __func__, rv_lock);
  }
  const bool is_process_in_of_sync =
      g_barrier_prevent_params_from_writing
          .m_is_process_in_of_ntp_time_synchronization;
  if (!g_barrier_prevent_params_from_writing
           .m_is_process_in_of_ntp_time_synchronization) {
    const int rv_lock2 = pthread_mutex_lock(&g_cond_for_rw.m_cond_base.m_mutex);
    if (g_cond_for_rw.m_is_in_rw == 0) {
      g_cond_for_rw.m_is_in_rw = -1;
    } else {
      while (true) {
        struct timespec abs_time = {0};
        bool calc_time = EsfClockManagerCalculateAbstimeInMonotonic(&abs_time,
                                                                    1000);

        if (!rv_lock2 && calc_time) {
          pthread_cond_timedwait(&g_cond_for_rw.m_cond_base.m_cond,
                                 &g_cond_for_rw.m_cond_base.m_mutex, &abs_time);
        } else {
          sleep((unsigned int)1);
        }

        if (g_cond_for_rw.m_is_in_rw == 0) {
          g_cond_for_rw.m_is_in_rw = -1;
          break;
        }
      }
    }
    if (!rv_lock2) {
      pthread_mutex_unlock(&g_cond_for_rw.m_cond_base.m_mutex);
    }
  }
  if (!rv_lock) {
    pthread_mutex_unlock(
        &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex);
  }

  return is_process_in_of_sync;
}

void EsfClockManagerLeaveReading(void) {
  const int rv_lock = pthread_mutex_lock(&g_cond_for_rw.m_cond_base.m_mutex);
  if (rv_lock) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_setting.c", __LINE__, __func__, rv_lock);
  }
  if (g_cond_for_rw.m_is_in_rw > 0) {
    g_cond_for_rw.m_is_in_rw--;
  }
  pthread_cond_broadcast(&g_cond_for_rw.m_cond_base.m_cond);
  if (!rv_lock) {
    pthread_mutex_unlock(&g_cond_for_rw.m_cond_base.m_mutex);
  }

  return;
}

void EsfClockManagerLeaveWriting(void) {
  const int rv_lock = pthread_mutex_lock(&g_cond_for_rw.m_cond_base.m_mutex);
  if (rv_lock) {
    WRITE_DLOG_WARN(MODULE_ID_SYSTEM,
                    "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                    "clock_manager_setting.c", __LINE__, __func__, rv_lock);
  }
  if (g_cond_for_rw.m_is_in_rw < 0) {
    g_cond_for_rw.m_is_in_rw++;
  }
  pthread_cond_broadcast(&g_cond_for_rw.m_cond_base.m_cond);
  if (!rv_lock) {
    pthread_mutex_unlock(&g_cond_for_rw.m_cond_base.m_mutex);
  }

  return;
}

EsfClockManagerReturnValue EsfClockManagerMarkCompletedSync(void) {
  if (pthread_mutex_lock(
          &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex)) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- pthread_mutex_lock failed.\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }
  g_barrier_prevent_params_from_writing
      .m_is_process_in_of_ntp_time_synchronization = false;
  pthread_mutex_unlock(
      &g_barrier_prevent_params_from_writing.m_mutex_base.m_mutex);
  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerGetParamsInternal(
    EsfClockManagerParams *const params_obj, const int tag) {
  if (params_obj == NULL) {
    return kClockManagerParamError;
  }

  bool load_flag = false;

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }

    if (g_params_in_volatile == NULL) {
      (void)pthread_mutex_unlock(&s_params_in_volatile_mutex);
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- g_params_in_volatile is NULL\n",
                       "clock_manager_setting.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    if (g_params_in_volatile->connect.hostname[0] == '\0' ||
        (strnlen(g_params_in_volatile->connect.hostname,
                 ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE) >=
         (size_t)ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE) ||
        g_params_in_volatile->common.raw_sync_interval.sync_interval < 0 ||
        g_params_in_volatile->common.raw_polling_time.polling_time < 0 ||
        g_params_in_volatile->skip_and_limit.raw_type.type >=
            (EsfClockManagerPsmParamsType)kClockManagerParamTypeNumMax ||
        g_params_in_volatile->skip_and_limit.raw_limit_packet_time
                .limit_packet_time < 0 ||
        g_params_in_volatile->skip_and_limit.raw_limit_rtc_correction_value
                .limit_rtc_correction_value < 0 ||
        g_params_in_volatile->skip_and_limit.raw_sanity_limit.sanity_limit <
            0 ||
        g_params_in_volatile->slew_setting.raw_type.type >=
            (EsfClockManagerPsmParamsType)kClockManagerParamTypeNumMax ||
        g_params_in_volatile->slew_setting.raw_stable_rtc_correction_value
                .stable_rtc_correction_value < 0 ||
        g_params_in_volatile->slew_setting.raw_stable_sync_number
                .stable_sync_number < 0) {
      load_flag = true;
    }

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }
  }

  EsfClockManagerParamsForPsm *tmp_params = NULL;
  EsfClockManagerReturnValue rv = kClockManagerSuccess;

  if (load_flag) {
    tmp_params = calloc(1, sizeof(*tmp_params));
    if (tmp_params == NULL) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- calloc failed\n",
                       "clock_manager_setting.c", __LINE__, __func__);
      return kClockManagerInternalError;
    }

    rv = EsfClockManagerLoadFromPsm(tmp_params, tag);
    if (rv != kClockManagerSuccess) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- EsfClockManagerLoadFromPsm failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv);
      free(tmp_params);
      return kClockManagerInternalError;
    }
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      free(tmp_params);
      return kClockManagerInternalError;
    }

    if (load_flag) {
      memcpy(g_params_in_volatile, tmp_params,
             sizeof(EsfClockManagerParamsForPsm));

      free(tmp_params);
      tmp_params = NULL;
    }

    rv = EsfClockManagerPsmParams2Params(params_obj, g_params_in_volatile);
    if (rv != kClockManagerSuccess) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:%s --- EsfClockManagerPsmParams2Params failed:%d\n",
          "clock_manager_setting.c", __LINE__, __func__, rv);
      (void)pthread_mutex_unlock(&s_params_in_volatile_mutex);
      return kClockManagerInternalError;
    }

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerSaveParamsInternal(void) {
  const EsfClockManagerParamsForPsmMask psm_mask = {
      {1}, {1, 1}, {1, 1, 1, 1}, {1, 1, 1}};

  EsfClockManagerParamsForPsm *tmp_params = calloc(1, sizeof(*tmp_params));
  if (tmp_params == NULL) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- calloc failed\n",
                     "clock_manager_setting.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      free(tmp_params);
      return kClockManagerInternalError;
    }

    memcpy(tmp_params, g_params_in_volatile,
           sizeof(EsfClockManagerParamsForPsm));

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      free(tmp_params);
      return kClockManagerInternalError;
    }
  }

  const EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerSave(g_psm_handle,
                                     (EsfParameterStorageManagerMask)&psm_mask,
                                     (EsfParameterStorageManagerData)tmp_params,
                                     &kClockManagerParamStructInfo, NULL);

  free(tmp_params);

  if (status != kEsfParameterStorageManagerStatusOk) {
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x810A);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, "%s-%d:%s --- PSM could not save:%d\n",
                     "clock_manager_setting.c", __LINE__, __func__, status);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerRegisterFactoryResetCb(void) {
  const EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerRegisterFactoryReset(
          EsfClockManagerOnFactoryReset, NULL,
          &g_factory_reset_registration_id);
  if (status != kEsfParameterStorageManagerStatusOk) {
    g_factory_reset_registration_id =
        ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID;
    WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x810C);
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- PSM could not register:%d\n",
                     "clock_manager_setting.c", __LINE__, __func__, status);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}

EsfClockManagerReturnValue EsfClockManagerUnregisterFactoryResetCb(void) {
  EsfParameterStorageManagerUnregisterFactoryReset(
      g_factory_reset_registration_id);
  g_factory_reset_registration_id =
      ESF_PARAMETER_STORAGE_MANAGER_INVALID_FACTORY_RESET_ID;
  return kClockManagerSuccess;
}

/**
 * Definitions of private functions --- i.e., functions given storage-class
 * specifier static.
 */

STATIC void EsfClockManagerFreeResource(void) {
  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
    }

    free(g_params_in_volatile);
    g_params_in_volatile = NULL;

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
    }
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
    }

    free(g_temporary_for_params);
    g_temporary_for_params = NULL;

    rv_mutex = pthread_mutex_unlock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
    }
  }

  if (g_psm_handle != ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE) {
    (void)EsfParameterStorageManagerClose(g_psm_handle);
    g_psm_handle = ESF_PARAMETER_STORAGE_MANAGER_INVALID_HANDLE;
  }

  return;
}

STATIC bool EsfClockManagerCheckParamConnect(
    const EsfClockManagerConnection *const connect,
    const EsfClockManagerConnectionMask *const mask_connect) {
  if (connect == NULL || mask_connect == NULL) {
    return false;
  }
  if (mask_connect->hostname) {
    if (strnlen(connect->hostname, ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE) >=
        (size_t)ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE) {
      return false;
    }
  }
  return true;
}

STATIC bool EsfClockManagerCheckParamCommon(
    const EsfClockManagerCommon *const common,
    const EsfClockManagerCommonMask *const mask_common) {
  if (common == NULL || mask_common == NULL) {
    return false;
  }
  if (mask_common->sync_interval) {
    if ((common->sync_interval < CLOCK_MANAGER_SYNC_INTERVAL_MIN) ||
        (common->sync_interval > CLOCK_MANAGER_SYNC_INTERVAL_MAX)) {
      return false;
    }
  }
  if (mask_common->polling_time) {
    if ((common->polling_time < CLOCK_MANAGER_POLLING_TIME_MIN) ||
        (common->polling_time > CLOCK_MANAGER_POLLING_TIME_MAX)) {
      return false;
    }
  }
  return true;
}

STATIC bool EsfClockManagerCheckParamSkipAndLimit(
    const EsfClockManagerSkipAndLimit *const skip_and_limit,
    const EsfClockManagerSkipAndLimitMask *const mask_skip_and_limit) {
  if (skip_and_limit == NULL || mask_skip_and_limit == NULL) {
    return false;
  }
  if (mask_skip_and_limit->type) {
    if (skip_and_limit->type < kClockManagerParamTypeOff) {
      return false;
    }
    if (skip_and_limit->type >= kClockManagerParamTypeNumMax) {
      return false;
    }
  }
  if (mask_skip_and_limit->limit_packet_time) {
    if ((skip_and_limit->limit_packet_time <
         CLOCK_MANAGER_LIMIT_PACKET_TIME_MIN) ||
        (skip_and_limit->limit_packet_time >
         CLOCK_MANAGER_LIMIT_PACKET_TIME_MAX)) {
      return false;
    }
  }
  if (mask_skip_and_limit->limit_rtc_correction_value) {
    if ((skip_and_limit->limit_rtc_correction_value <
         CLOCK_MANAGER_RTC_CORRECT_LIMIT_MIN) ||
        (skip_and_limit->limit_rtc_correction_value >
         CLOCK_MANAGER_RTC_CORRECT_LIMIT_MAX)) {
      return false;
    }
  }
  if (mask_skip_and_limit->sanity_limit) {
    if ((skip_and_limit->sanity_limit < CLOCK_MANAGER_SANITY_LIMIT_MIN) ||
        (skip_and_limit->sanity_limit > CLOCK_MANAGER_SANITY_LIMIT_MAX)) {
      return false;
    }
  }
  return true;
}

STATIC bool EsfClockManagerCheckParamSlewParam(
    const EsfClockManagerSlewParam *const slew_setting,
    const EsfClockManagerSlewParamMask *const mask_slew_setting) {
  if (slew_setting == NULL || mask_slew_setting == NULL) {
    return false;
  }
  if (mask_slew_setting->type) {
    if (slew_setting->type < kClockManagerParamTypeOff) {
      return false;
    }
    if (slew_setting->type >= kClockManagerParamTypeNumMax) {
      return false;
    }
  }
  if (mask_slew_setting->stable_rtc_correction_value) {
    if ((slew_setting->stable_rtc_correction_value <
         CLOCK_MANAGER_STABLE_RTC_MIN) ||
        (slew_setting->stable_rtc_correction_value >
         CLOCK_MANAGER_STABLE_RTC_MAX)) {
      return false;
    }
  }
  if (mask_slew_setting->stable_sync_number) {
    if ((slew_setting->stable_sync_number <
         CLOCK_MANAGER_STABLE_SYNC_CONT_MIN) ||
        (slew_setting->stable_sync_number >
         CLOCK_MANAGER_STABLE_SYNC_CONT_MAX)) {
      return false;
    }
  }
  return true;
}

STATIC bool EsfClockManagerCheckParam(
    const EsfClockManagerParams *const data,
    const EsfClockManagerParamsMask *const mask) {
  if (data == NULL || mask == NULL) {
    return false;
  }
  if (!EsfClockManagerCheckParamConnect(&(data->connect), &mask->connect)) {
    return false;
  }
  if (!EsfClockManagerCheckParamCommon(&(data->common), &mask->common)) {
    return false;
  }
  if (!EsfClockManagerCheckParamSkipAndLimit(&(data->skip_and_limit),
                                             &mask->skip_and_limit)) {
    return false;
  }
  if (!EsfClockManagerCheckParamSlewParam(&(data->slew_setting),
                                          &mask->slew_setting)) {
    return false;
  }

  return true;
}

STATIC bool EsfClockManagerIsHostnameMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, connect.hostname, mask);
}

STATIC bool EsfClockManagerIsSyncIntervalMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, common.sync_interval, mask);
}

STATIC bool EsfClockManagerIsPollingTimeMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, common.polling_time, mask);
}

STATIC bool EsfClockManagerIsTypeMaskInSkipAndLimitEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, skip_and_limit.type, mask);
}

STATIC bool EsfClockManagerIsLimitPacketTimeMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, skip_and_limit.limit_packet_time, mask);
}

STATIC bool EsfClockManagerIsLimitRtcCorrectionValueMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, skip_and_limit.limit_rtc_correction_value,
      mask);
}

STATIC bool EsfClockManagerIsSanityLimitMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, skip_and_limit.sanity_limit, mask);
}

STATIC bool EsfClockManagerIsTypeMaskInSlewParamEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, slew_setting.type, mask);
}

STATIC bool EsfClockManagerIsStableRtcCorrectionValueMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, slew_setting.stable_rtc_correction_value,
      mask);
}

STATIC bool EsfClockManagerIsStableSyncNumberMaskEnabled(
    EsfParameterStorageManagerMask mask) {
  return ESF_PARAMETER_STORAGE_MANAGER_MASK_IS_ENABLED(
      EsfClockManagerParamsMask, slew_setting.stable_sync_number, mask);
}

STATIC EsfParameterStorageManagerStatus
EsfClockManagerOnFactoryReset(void *private_data) {
  NOT_USED(private_data);

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
    }

    EsfClockManagerGetPsmDefault(g_params_in_volatile,
                                 kEsfParameterStorageManagerItemNTPServer);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile, kEsfParameterStorageManagerItemNTPSyncInterval);
    EsfClockManagerGetPsmDefault(g_params_in_volatile,
                                 kEsfParameterStorageManagerItemNTPPollingTime);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile, kEsfParameterStorageManagerItemSkipModeSettings);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile, kEsfParameterStorageManagerItemLimitPacketTime);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile,
        kEsfParameterStorageManagerItemLimitRTCCorrectionValue);
    EsfClockManagerGetPsmDefault(g_params_in_volatile,
                                 kEsfParameterStorageManagerItemSanityLimit);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile, kEsfParameterStorageManagerItemSlewModeSettings);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile,
        kEsfParameterStorageManagerItemStableRTCCorrectionValue);
    EsfClockManagerGetPsmDefault(
        g_params_in_volatile, kEsfParameterStorageManagerItemStableSyncNumber);

    if (rv_mutex == 0) {
      rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
      if (rv_mutex != 0) {
        WRITE_DLOG_ERROR(
            MODULE_ID_SYSTEM, "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
            "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      }
    }
  }

  (void)PlClockManagerDeleteConfFiles();

  return kEsfParameterStorageManagerStatusOk;
}

STATIC EsfClockManagerReturnValue EsfClockManagerLoadFromPsm(
    EsfClockManagerParamsForPsm *const psm_obj, const int tag) {
  const EsfClockManagerParamsForPsmMask psm_mask = {
      {1}, {1, 1}, {1, 1, 1, 1}, {1, 1, 1}};

  if (psm_obj == NULL) {
    return kClockManagerParamError;
  }
  if (tag != CALLER_IS_INIT_SETTING) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- Why was this function called?<tag:%d>\n",
                        "clock_manager_setting.c", __LINE__, __func__, tag);
  }
  const EsfParameterStorageManagerStatus status =
      EsfParameterStorageManagerLoad(g_psm_handle,
                                     (EsfParameterStorageManagerMask)&psm_mask,
                                     (EsfParameterStorageManagerData)psm_obj,
                                     &kClockManagerParamStructInfo, NULL);
  if (status != kEsfParameterStorageManagerStatusOk) {
    switch (tag) {
      case CALLER_IS_INIT_SETTING:
        WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (uint16_t)0x8100);
        WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                         "%s-%d:%s --- PSM could not load:%d\n",
                         "clock_manager_setting.c", __LINE__, __func__, status);
        break;
      case CALLER_IS_GET_PARAMS:  // fallthrough
      case CALLER_IS_START_INTERNAL:
        WRITE_ELOG_CRITICAL(MODULE_ID_SYSTEM, (uint16_t)0x8101);
        WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                            "%s-%d:%s --- Why was this function "
                            "called?<tag:%d> PSM could not load:%d\n",
                            "clock_manager_setting.c", __LINE__, __func__, tag,
                            status);
        break;
      default:
        break;
    }
    EsfClockManagerGetPsmDefault(psm_obj,
                                 kEsfParameterStorageManagerItemNTPServer);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemNTPSyncInterval);
    EsfClockManagerGetPsmDefault(psm_obj,
                                 kEsfParameterStorageManagerItemNTPPollingTime);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemSkipModeSettings);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemLimitPacketTime);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemLimitRTCCorrectionValue);
    EsfClockManagerGetPsmDefault(psm_obj,
                                 kEsfParameterStorageManagerItemSanityLimit);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemSlewModeSettings);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemStableRTCCorrectionValue);
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemStableSyncNumber);
  } else {
    EsfClockManagerCheckPsmDataThenSubstituteDefaultForIfNecessary(psm_obj);
  }

  return kClockManagerSuccess;
}

STATIC void EsfClockManagerCheckPsmConnectThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  if (psm_obj == NULL) {
    return;
  }

  if (psm_obj->connect.hostname[0] == '\0' ||
      (strnlen(&(psm_obj->connect.hostname[0]),
               ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE) >=
       (size_t)ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE)) {
    EsfClockManagerGetPsmDefault(psm_obj,
                                 kEsfParameterStorageManagerItemNTPServer);
  }

  return;
}

STATIC void EsfClockManagerCheckPsmCommonThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  if (psm_obj == NULL) {
    return;
  }

  if ((psm_obj->common.raw_sync_interval.size == 0) ||
      ((psm_obj->common.raw_sync_interval.size != 0) &&
       ((psm_obj->common.raw_sync_interval.sync_interval <
         CLOCK_MANAGER_SYNC_INTERVAL_MIN) ||
        (psm_obj->common.raw_sync_interval.sync_interval >
         CLOCK_MANAGER_SYNC_INTERVAL_MAX)))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemNTPSyncInterval);
  }

  if ((psm_obj->common.raw_polling_time.size == 0) ||
      ((psm_obj->common.raw_polling_time.size != 0) &&
       ((psm_obj->common.raw_polling_time.polling_time <
         CLOCK_MANAGER_POLLING_TIME_MIN) ||
        (psm_obj->common.raw_polling_time.polling_time >
         CLOCK_MANAGER_POLLING_TIME_MAX)))) {
    EsfClockManagerGetPsmDefault(psm_obj,
                                 kEsfParameterStorageManagerItemNTPPollingTime);
  }

  return;
}

STATIC void
EsfClockManagerCheckPsmSub1SkipAndLimitThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  if (psm_obj == NULL) {
    return;
  }

  if ((psm_obj->skip_and_limit.raw_type.size == 0) ||
      ((psm_obj->skip_and_limit.raw_type.size != 0) &&
       (psm_obj->skip_and_limit.raw_type.type >=
        (EsfClockManagerPsmParamsType)kClockManagerParamTypeNumMax))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemSkipModeSettings);
  }

  if ((psm_obj->skip_and_limit.raw_limit_packet_time.size == 0) ||
      ((psm_obj->skip_and_limit.raw_limit_packet_time.size != 0) &&
       ((psm_obj->skip_and_limit.raw_limit_packet_time.limit_packet_time <
         CLOCK_MANAGER_LIMIT_PACKET_TIME_MIN) ||
        (psm_obj->skip_and_limit.raw_limit_packet_time.limit_packet_time >
         CLOCK_MANAGER_LIMIT_PACKET_TIME_MAX)))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemLimitPacketTime);
  }

  return;
}

STATIC void
EsfClockManagerCheckPsmSub2SkipAndLimitThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  if (psm_obj == NULL) {
    return;
  }

  if ((psm_obj->skip_and_limit.raw_limit_rtc_correction_value.size == 0) ||
      ((psm_obj->skip_and_limit.raw_limit_rtc_correction_value.size != 0) &&
       ((psm_obj->skip_and_limit.raw_limit_rtc_correction_value
             .limit_rtc_correction_value <
         CLOCK_MANAGER_RTC_CORRECT_LIMIT_MIN) ||
        (psm_obj->skip_and_limit.raw_limit_rtc_correction_value
             .limit_rtc_correction_value >
         CLOCK_MANAGER_RTC_CORRECT_LIMIT_MAX)))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemLimitRTCCorrectionValue);
  }

  if ((psm_obj->skip_and_limit.raw_sanity_limit.size == 0) ||
      ((psm_obj->skip_and_limit.raw_sanity_limit.size != 0) &&
       ((psm_obj->skip_and_limit.raw_sanity_limit.sanity_limit <
         CLOCK_MANAGER_SANITY_LIMIT_MIN) ||
        (psm_obj->skip_and_limit.raw_sanity_limit.sanity_limit >
         CLOCK_MANAGER_SANITY_LIMIT_MAX)))) {
    EsfClockManagerGetPsmDefault(psm_obj,
                                 kEsfParameterStorageManagerItemSanityLimit);
  }

  return;
}

STATIC void
EsfClockManagerCheckPsmSkipAndLimitThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  if (psm_obj == NULL) {
    return;
  }

  EsfClockManagerCheckPsmSub1SkipAndLimitThenSubstituteDefaultForIfNecessary(
      psm_obj);
  EsfClockManagerCheckPsmSub2SkipAndLimitThenSubstituteDefaultForIfNecessary(
      psm_obj);

  return;
}

STATIC void
EsfClockManagerCheckPsmSlewSettingThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  if (psm_obj == NULL) {
    return;
  }

  if ((psm_obj->slew_setting.raw_type.size == 0) ||
      ((psm_obj->slew_setting.raw_type.size != 0) &&
       (psm_obj->slew_setting.raw_type.type >=
        (EsfClockManagerPsmParamsType)kClockManagerParamTypeNumMax))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemSlewModeSettings);
  }

  if ((psm_obj->slew_setting.raw_stable_rtc_correction_value.size == 0) ||
      ((psm_obj->slew_setting.raw_stable_rtc_correction_value.size != 0) &&
       ((psm_obj->slew_setting.raw_stable_rtc_correction_value
             .stable_rtc_correction_value < CLOCK_MANAGER_STABLE_RTC_MIN) ||
        (psm_obj->slew_setting.raw_stable_rtc_correction_value
             .stable_rtc_correction_value > CLOCK_MANAGER_STABLE_RTC_MAX)))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemStableRTCCorrectionValue);
  }

  if ((psm_obj->slew_setting.raw_stable_sync_number.size == 0) ||
      ((psm_obj->slew_setting.raw_stable_sync_number.size != 0) &&
       ((psm_obj->slew_setting.raw_stable_sync_number.stable_sync_number <
         CLOCK_MANAGER_STABLE_SYNC_CONT_MIN) ||
        (psm_obj->slew_setting.raw_stable_sync_number.stable_sync_number >
         CLOCK_MANAGER_STABLE_SYNC_CONT_MAX)))) {
    EsfClockManagerGetPsmDefault(
        psm_obj, kEsfParameterStorageManagerItemStableSyncNumber);
  }

  return;
}

STATIC void EsfClockManagerCheckPsmDataThenSubstituteDefaultForIfNecessary(
    EsfClockManagerParamsForPsm *const psm_obj) {
  EsfClockManagerCheckPsmConnectThenSubstituteDefaultForIfNecessary(psm_obj);
  EsfClockManagerCheckPsmCommonThenSubstituteDefaultForIfNecessary(psm_obj);
  EsfClockManagerCheckPsmSkipAndLimitThenSubstituteDefaultForIfNecessary(
      psm_obj);
  EsfClockManagerCheckPsmSlewSettingThenSubstituteDefaultForIfNecessary(
      psm_obj);

  return;
}

STATIC void EsfClockManagerGetPsmDefault(
    EsfClockManagerParamsForPsm *const psm_obj,
    EsfParameterStorageManagerItemID item_id) {
  switch (item_id) {
    case kEsfParameterStorageManagerItemNTPServer:
      snprintf(&(psm_obj->connect.hostname[0]),
               ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE, "%s",
               CLOCK_MANAGER_DEFAULT_NTP_SERVER);
      break;
    case kEsfParameterStorageManagerItemNTPSyncInterval:
      psm_obj->common.raw_sync_interval.sync_interval =
          CLOCK_MANAGER_SYNC_INTERVAL_DEF;
      break;
    case kEsfParameterStorageManagerItemNTPPollingTime:
      psm_obj->common.raw_polling_time.polling_time =
          CLOCK_MANAGER_POLLING_TIME_DEF;
      break;
    case kEsfParameterStorageManagerItemSkipModeSettings:
      psm_obj->skip_and_limit.raw_type.type =
          (EsfClockManagerPsmParamsType)kClockManagerParamTypeOff;
      break;
    case kEsfParameterStorageManagerItemLimitPacketTime:
      psm_obj->skip_and_limit.raw_limit_packet_time.limit_packet_time =
          CLOCK_MANAGER_LIMIT_PACKET_TIME_DEF;
      break;
    case kEsfParameterStorageManagerItemLimitRTCCorrectionValue:
      psm_obj->skip_and_limit.raw_limit_rtc_correction_value
          .limit_rtc_correction_value = CLOCK_MANAGER_RTC_CORRECT_LIMIT_DEF;
      break;
    case kEsfParameterStorageManagerItemSanityLimit:
      psm_obj->skip_and_limit.raw_sanity_limit.sanity_limit =
          CLOCK_MANAGER_SANITY_LIMIT_DEF;
      break;
    case kEsfParameterStorageManagerItemSlewModeSettings:
      psm_obj->slew_setting.raw_type.type =
          (EsfClockManagerPsmParamsType)kClockManagerParamTypeOff;
      break;
    case kEsfParameterStorageManagerItemStableRTCCorrectionValue:
      psm_obj->slew_setting.raw_stable_rtc_correction_value
          .stable_rtc_correction_value = CLOCK_MANAGER_STABLE_RTC_DEF;
      break;
    case kEsfParameterStorageManagerItemStableSyncNumber:
      psm_obj->slew_setting.raw_stable_sync_number.stable_sync_number =
          CLOCK_MANAGER_STABLE_SYNC_CONT_DEF;
      break;
    default:
      break;
  }

  return;
}

STATIC EsfClockManagerReturnValue EsfClockManagerSubstituteForVolatileObj(
    const EsfClockManagerParams *const data,
    const EsfClockManagerParamsMask *const mask) {
  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }

    EsfClockManagerReturnValue rv =
        EsfClockManagerParams2PsmParams(g_temporary_for_params, data);
    if (rv != kClockManagerSuccess) {
      WRITE_DLOG_ERROR(
          MODULE_ID_SYSTEM,
          "%s-%d:%s --- EsfClockManagerParams2PsmParams failed:%d\n",
          "clock_manager_setting.c", __LINE__, __func__, rv);
      (void)pthread_mutex_unlock(&s_temporary_for_params_mutex);
      return kClockManagerInternalError;
    }

    rv_mutex = pthread_mutex_unlock(&s_temporary_for_params_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }
  }

  {  // Lock the mutex to access shared resources
    int rv_mutex = pthread_mutex_lock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_lock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }

    if ((g_params_in_volatile == NULL) || (g_temporary_for_params == NULL)) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- g_params_in_volatile or "
                       "g_temporary_for_params is NULL\n",
                       "clock_manager_setting.c", __LINE__, __func__);
      (void)pthread_mutex_unlock(&s_params_in_volatile_mutex);
      return kClockManagerInternalError;
    }

    if (mask->connect.hostname) {
      snprintf(g_params_in_volatile->connect.hostname,
               ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE, "%s",
               g_temporary_for_params->connect.hostname);
      EsfClockManagerCheckPsmConnectThenSubstituteDefaultForIfNecessary(
          g_params_in_volatile);
    }
    if (mask->common.sync_interval) {
      g_params_in_volatile->common.raw_sync_interval.sync_interval =
          g_temporary_for_params->common.raw_sync_interval.sync_interval;
    }
    if (mask->common.polling_time) {
      g_params_in_volatile->common.raw_polling_time.polling_time =
          g_temporary_for_params->common.raw_polling_time.polling_time;
    }
    if (mask->skip_and_limit.type) {
      g_params_in_volatile->skip_and_limit.raw_type.type =
          g_temporary_for_params->skip_and_limit.raw_type.type;
    }
    if (mask->skip_and_limit.limit_packet_time) {
      g_params_in_volatile->skip_and_limit.raw_limit_packet_time
          .limit_packet_time = g_temporary_for_params->skip_and_limit
                                   .raw_limit_packet_time.limit_packet_time;
    }
    if (mask->skip_and_limit.limit_rtc_correction_value) {
      g_params_in_volatile->skip_and_limit.raw_limit_rtc_correction_value
          .limit_rtc_correction_value =
          g_temporary_for_params->skip_and_limit.raw_limit_rtc_correction_value
              .limit_rtc_correction_value;
    }
    if (mask->skip_and_limit.sanity_limit) {
      g_params_in_volatile->skip_and_limit.raw_sanity_limit.sanity_limit =
          g_temporary_for_params->skip_and_limit.raw_sanity_limit.sanity_limit;
    }
    if (mask->slew_setting.type) {
      g_params_in_volatile->slew_setting.raw_type.type =
          g_temporary_for_params->slew_setting.raw_type.type;
    }
    if (mask->slew_setting.stable_rtc_correction_value) {
      g_params_in_volatile->slew_setting.raw_stable_rtc_correction_value
          .stable_rtc_correction_value =
          g_temporary_for_params->slew_setting.raw_stable_rtc_correction_value
              .stable_rtc_correction_value;
    }
    if (mask->slew_setting.stable_sync_number) {
      g_params_in_volatile->slew_setting.raw_stable_sync_number
          .stable_sync_number = g_temporary_for_params->slew_setting
                                    .raw_stable_sync_number.stable_sync_number;
    }

    rv_mutex = pthread_mutex_unlock(&s_params_in_volatile_mutex);
    if (rv_mutex != 0) {
      WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                       "%s-%d:%s --- pthread_mutex_unlock failed:%d\n",
                       "clock_manager_setting.c", __LINE__, __func__, rv_mutex);
      return kClockManagerInternalError;
    }
  }

  return kClockManagerSuccess;
}

STATIC EsfClockManagerReturnValue
EsfClockManagerParams2PsmParams(EsfClockManagerParamsForPsm *const psm_params,
                                const EsfClockManagerParams *const params) {
  if (psm_params == NULL || params == NULL) {
    return kClockManagerParamError;
  }

  strncpy(&(psm_params->connect.hostname[0]), &(params->connect.hostname[0]),
          ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE);

  psm_params->common.raw_sync_interval.sync_interval =
      params->common.sync_interval;
  psm_params->common.raw_polling_time.polling_time =
      params->common.polling_time;

  psm_params->skip_and_limit.raw_type.type =
      EsfClockManagerParamsType2PsmParamsType(params->skip_and_limit.type);
  psm_params->skip_and_limit.raw_limit_packet_time.limit_packet_time =
      params->skip_and_limit.limit_packet_time;
  psm_params->skip_and_limit.raw_limit_rtc_correction_value
      .limit_rtc_correction_value =
      params->skip_and_limit.limit_rtc_correction_value;
  psm_params->skip_and_limit.raw_sanity_limit.sanity_limit =
      params->skip_and_limit.sanity_limit;

  psm_params->slew_setting.raw_type.type =
      EsfClockManagerParamsType2PsmParamsType(params->slew_setting.type);
  psm_params->slew_setting.raw_stable_rtc_correction_value
      .stable_rtc_correction_value =
      params->slew_setting.stable_rtc_correction_value;
  psm_params->slew_setting.raw_stable_sync_number.stable_sync_number =
      params->slew_setting.stable_sync_number;

  return kClockManagerSuccess;
}

STATIC EsfClockManagerPsmParamsType
EsfClockManagerParamsType2PsmParamsType(EsfClockManagerParamType type) {
  EsfClockManagerPsmParamsType rv = (EsfClockManagerPsmParamsType)0;
  switch (type) {
    case kClockManagerParamTypeDefault:
      rv = (EsfClockManagerPsmParamsType)1;
      break;
    case kClockManagerParamTypeCustom:
      rv = (EsfClockManagerPsmParamsType)2;
      break;
    case kClockManagerParamTypeNumMax:
      rv = (EsfClockManagerPsmParamsType)3;
      break;
    case kClockManagerParamTypeOff:  // fallthrough
    default:
      break;
  }
  return rv;
}

STATIC EsfClockManagerReturnValue EsfClockManagerPsmParams2Params(
    EsfClockManagerParams *const params,
    const EsfClockManagerParamsForPsm *const psm_params) {
  if (params == NULL || psm_params == NULL) {
    return kClockManagerParamError;
  }

  strncpy(&(params->connect.hostname[0]), &(psm_params->connect.hostname[0]),
          ESF_CLOCK_MANAGER_NTPADDR_MAX_SIZE);

  params->common.sync_interval =
      psm_params->common.raw_sync_interval.sync_interval;
  params->common.polling_time =
      psm_params->common.raw_polling_time.polling_time;

  params->skip_and_limit.type =
      (EsfClockManagerParamType)psm_params->skip_and_limit.raw_type.type;
  params->skip_and_limit.limit_packet_time =
      psm_params->skip_and_limit.raw_limit_packet_time.limit_packet_time;
  params->skip_and_limit.limit_rtc_correction_value =
      psm_params->skip_and_limit.raw_limit_rtc_correction_value
          .limit_rtc_correction_value;
  params->skip_and_limit.sanity_limit =
      psm_params->skip_and_limit.raw_sanity_limit.sanity_limit;

  params->slew_setting.type =
      (EsfClockManagerParamType)psm_params->slew_setting.raw_type.type;
  params->slew_setting.stable_rtc_correction_value =
      psm_params->slew_setting.raw_stable_rtc_correction_value
          .stable_rtc_correction_value;
  params->slew_setting.stable_sync_number =
      psm_params->slew_setting.raw_stable_sync_number.stable_sync_number;

  return kClockManagerSuccess;
}
