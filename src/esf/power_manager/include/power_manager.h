/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_POWER_MANAGER_POWER_MANAGER_H_
#define ESF_POWER_MANAGER_POWER_MANAGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// An enumerated type that defines the result of API execution.
typedef enum EsfPwrMgrError {
  kEsfPwrMgrOk,                      // Processing Success.
  kEsfPwrMgrErrorInvalidArgument,    // Argument error.
  kEsfPwrMgrErrorResourceExhausted,  // Insufficient memory error.
  kEsfPwrMgrErrorInternal,           // Internal processing error.
  kEsfPwrMgrErrorAlreadyRunning,     // Return value during processing.
  kEsfPwrMgrErrorStatus,             // State error.
  kEsfPwrMgrErrorExternal,           // External API execution error.
  kEsfPwrMgrErrorTimeout,            // Timeout occurred.
  kEsfPwrMgrErrorUnsupportedApi,     // Unsupported API
  kEsfPwrMgrErrorWaitReboot,         // Wait WDT Ignition
} EsfPwrMgrError;

typedef enum EsfPwrMgrSupplyType {
  kEsfPwrMgrSupplyTypeUnknown = -1,
  kEsfPwrMgrSupplyTypePoE,
  kEsfPwrMgrSupplyTypeUsb,
  kEsfPwrMgrSupplyTypeDcPlug,
  kEsfPwrMgrSupplyTypePrimaryBattery,
  kEsfPwrMgrSupplyTypeSecondaryBattery,
  kEsfPwrMgrSupplyTypeMax
} EsfPwrMgrSupplyType;

typedef enum EsfPwrMgrResetCause {
  kEsfPwrMgrResetCauseUnknown = -1,
  kEsfPwrMgrResetCauseSysChipPowerOnReset = 0,
  kEsfPwrMgrResetCauseSysBrownOut,
  kEsfPwrMgrResetCauseCoreSoft,
  kEsfPwrMgrResetCauseCoreDeepSleep,
  kEsfPwrMgrResetCauseWDT,
  kEsfPwrMgrResetCauseMax
} EsfPwrMgrResetCause;

typedef enum EsfPwrMgrRebootType {
  EsfPwrMgrRebootTypeSW,
  EsfPwrMgrRebootTypeHW
} EsfPwrMgrRebootType;

// A define periodic addition cycle (in hours) for HoursMeter.
#define ESF_POWER_MANAGER_HOURS_METER_ADD_INTERVAL (1)

#ifdef CONFIG_STACK_COLORATION
#define ESF_POWER_MANAGER_EXCEPTION_INFO_SIZE   (18158)
#else
#define ESF_POWER_MANAGER_EXCEPTION_INFO_SIZE   (18141)
#endif

struct EsfPwrMgrExceptionInfo;

// """Start Power Manager.

// Description: Initializes the internal state.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorInternal: Internal processing error.
//    kEsfPwrMgrErrorExternal: Error in ParameterStorageManager operation.
//    kEsfPwrMgrErrorExternal: Error occurs in HAL operation.
EsfPwrMgrError EsfPwrMgrStart(void);

// """Exit Power Manager.

// Description: Internal state is set to Stop state.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorInternal: Internal processing error.
//    kEsfPwrMgrErrorExternal: Error in ParameterStorageManager operation.
//    kEsfPwrMgrErrorExternal: Error occurs in HAL operation.
EsfPwrMgrError EsfPwrMgrStop(void);

// """Exit Power Manager and wait WDT Ignition.

// Description: Internal state is set to Stop state.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorInternal: Internal processing error.
//    kEsfPwrMgrErrorExternal: Error in ParameterStorageManager operation.
//    kEsfPwrMgrErrorExternal: Error occurs in PL operation.
EsfPwrMgrError EsfPwrMgrStopForReboot(void);

// """Initiate system reboot.

// Description:
// Notifies SSF(main) of a restart event and notifies it of the opportunity to
// execute the necessary processing.
// Notify SSF(Led Manager) of the status change.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorAlreadyRunning: Return value during processing.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrPrepareReboot(void);

// """Perform a system reboot.

// Description:
// HAL(SystemControl) to perform a system reboot.
// This API can be executed even when Power Manager is stopped.
// If this API is successful, the device will be restarted and will not respond.

// Args:
//    [In] reboot_type(EsfPwrMgrRebootType) : SW Reset or HW reset

// Returns:
//    None.
void EsfPwrMgrExecuteRebootEx(EsfPwrMgrRebootType reboot_type);

// """Perform a system reboot.

// Description:
// HAL(SystemControl) to perform a system reboot.
// This API can be executed even when Power Manager is stopped.
// If this API is successful, the device will be restarted and will not respond.

// Args:
//    void

// Returns:
//    None.
// TODO:Scheduled for deletion
void EsfPwrMgrExecuteReboot(void);

// """Initiate system shutdown.

// Description:
// Notifies SSF(main) of the shutdown and notifies it of the opportunity to
// execute the required process.
// Notify SSF(Led Manager) of the status change.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorAlreadyRunning: Return value during processing.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrPrepareShutdown(void);

// """Perform a system shutdown.

// Description:
// HAL(SystemControl) to perform a system shutdown.
// This API can be executed even when Power Manager is stopped.
// If this API is successful, the device will be restarted and will not respond.

// Args:
//    void

// Returns:
//    None.
void EsfPwrMgrExecuteShutdown(void);

// """Obtains operating voltage information.

// Description:
// Obtains operating voltage information from HAL(T.B.D) and responds.

// Args:
//    [OUT] voltage(int32_t *) : Operating voltage information.
//                               NULL is not acceptable.

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorInvalidArgument: Argument error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrGetVoltage(int32_t *voltage);

// """Get current value of Hours meter.

// Description: Retrieve and respond to Hours meter.

// Args:
//    [OUT] hours (int32_t *): Current value of Hours meter.
//                             NULL is not acceptable.

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorInvalidArgument: Argument error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrHoursMeterGetValue(int32_t *hours);

// """Terminate WDT

// Description: Call WDT Terminate API.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
EsfPwrMgrError EsfPwrMgrWdtTerminate(void);

// """Get current Supply Type

// Description: Call Get Supply Type API.

// Args:
//    [OUT] supply_type (EsfPwrMgrSupplyType *): Current Supply Type.
//                                               NULL is not acceptable.

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorInvalidArgument: Argument error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
//    kEsfPwrMgrErrorUnsupportedApi: Not support api error
EsfPwrMgrError EsfPwrMgrGetSupplyType(EsfPwrMgrSupplyType *supply_type);

// """Send KeepAlive to WDT

// Description: Send KeepAlive to WDT.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrWdtKeepAlive(void);

// """Get Exception Information

// Description: Get Exception Information.

// Args:
//    [OUT] info (struct EsfPwrMgrExceptionInfo **): Exception Information.
//                                                   NULL is not acceptable.
//    [OUT] info_size (uint32_t *): Exception Information Size.
//                                  NULL is not acceptable.

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorInvalidArgument: Argument error.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrGetExceptionInfo(struct EsfPwrMgrExceptionInfo **info,
                                         uint32_t *info_size);

// """Convert Exception Information

// Description:
// Convert Exception Information to string.
// If the dst_size argument is smaller than the size of the exception
// information, not all exception information will be retrieved.
// ESF_POWER_MANAGER_EXCEPTION_INFO_SIZE are required to retrieve
// all exception information.

// Args:
//    [IN]  info (struct EsfPwrMgrExceptionInfo *): Exception Information.
//                                                  NULL is not acceptable.
//    [OUT] dst (char *): Exception Information String.
//                        NULL is not acceptable.
//    [IN]  dst_size (uint32_t): Exception Information String size.
//                               0 is not acceptable.

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorInvalidArgument: Argument error.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorTimeout: Timeout occurred.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrConvExceptionInfo(
  struct EsfPwrMgrExceptionInfo *info, char *dst, uint32_t dst_size);

// """Clear Exception Information

// Description: Clear Exception Information.

// Args:
//    void

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorStatus: State error.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrClearExceptionInfo(void);

// """Get Reset Cause

// Description: Call Get Reset Cause API.

// Args:
//    [OUT] reset_cause (EsfPwrMgrResetCause *): Reset Cause.
//                                               NULL is not acceptable.

// Returns:
//    kEsfPwrMgrOk: Processing Success.
//    kEsfPwrMgrErrorInvalidArgument: Argument error.
//    kEsfPwrMgrErrorExternal: External API execution error.
//    kEsfPwrMgrErrorInternal: Internal processing error.
EsfPwrMgrError EsfPwrMgrGetResetCause(EsfPwrMgrResetCause *reset_cause);

EsfPwrMgrError EsfPwrMgrSwWdtStart(uint32_t id);
EsfPwrMgrError EsfPwrMgrSwWdtStop(uint32_t id);
EsfPwrMgrError EsfPwrMgrSwWdtKeepalive(uint32_t id);
EsfPwrMgrError EsfPwrMgrExecMigration(void);

#ifdef __cplusplus
}
#endif

#endif  // ESF_POWER_MANAGER_POWER_MANAGER_H_
