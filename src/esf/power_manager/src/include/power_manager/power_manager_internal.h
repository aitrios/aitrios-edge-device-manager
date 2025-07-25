/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef ESF_POWER_MANAGER_POWER_MANAGER_INTERNAL_H_
#define ESF_POWER_MANAGER_POWER_MANAGER_INTERNAL_H_

#include <pthread.h>

#include "parameter_storage_manager.h"
#include "power_manager.h"
#include "utility_timer.h"

// HoursMeter value size
#define ESF_PWR_MGR_HOURS_METER_MAX_SIZE (16)

// PowerManager Status
typedef enum EsfPwrMgrStatus {
  kEsfPwrMgrStatusStop = 0,
  kEsfPwrMgrStatusStart,
  kEsfPwrMgrStatusReboot,
  kEsfPwrMgrStatusShutdown,
  kEsfPwrMgrStatusWaitWDTIgnition
} EsfPwrMgrStatus;


// A structure that defines the internal resources of PowerManager.
typedef struct EsfPwrMgrResource {
  // the handle of ParameterStorageManager
  EsfParameterStorageManagerHandle storage_handle;
  // the handle of hal timer
  UtilityTimerHandle timer_handle;
  // Current value of hours meter
  int32_t hours_meter;
} EsfPwrMgrResource;

// A structure that defines an hour meter data structure.
typedef struct EsfPwrMgrHoursMeter {
  char hours_meter_str[ESF_PWR_MGR_HOURS_METER_MAX_SIZE];
} EsfPwrMgrHoursMeter;

// A structure that defines an hour meter mask structure.
typedef struct EsfPwrMgrHoursMeterMask {
  uint8_t hours_meter : 1;  // Hours meter mask information
} EsfPwrMgrHoursMeterMask;

typedef enum EsfPwrMgrElogId {
// Error Id
  // EsfPwrMgr
  kEsfPwrMgrElogErrorId0x00EsfPwrMgrCommonMutexLock = 0x00,
  kEsfPwrMgrElogErrorId0x01EsfPwrMgrCommonMutexUnlock,
  kEsfPwrMgrElogErrorId0x02EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x03EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x04EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x05EsfPwrMgr,  // not used
  kEsfPwrMgrElogErrorId0x06EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x07EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x08EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x09EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x0aEsfPwrMgr,
  kEsfPwrMgrElogErrorId0x0bEsfPwrMgr,
  kEsfPwrMgrElogErrorId0x0cEsfPwrMgr,
  kEsfPwrMgrElogErrorId0x0dEsfPwrMgr,
  kEsfPwrMgrElogErrorId0x0eEsfPwrMgr,
  kEsfPwrMgrElogErrorId0x0fEsfPwrMgr,
  kEsfPwrMgrElogErrorId0x10EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x11EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x12EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x13EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x14EsfPwrMgr,
  kEsfPwrMgrElogErrorId0x15EsfPwrMgr,
  // UtilityTimer
  kEsfPwrMgrElogErrorId0x20UtlTimer = 0x20,
  kEsfPwrMgrElogErrorId0x21UtlTimer,
  kEsfPwrMgrElogErrorId0x22UtlTimer,
  kEsfPwrMgrElogErrorId0x23UtlTimer,
  // EsfParameterStorageManager
  kEsfPwrMgrElogErrorId0x30EsfPSM = 0x30,
  kEsfPwrMgrElogErrorId0x31EsfPSM,
  kEsfPwrMgrElogErrorId0x32EsfPSM,
  kEsfPwrMgrElogErrorId0x33EsfPSM,
  // EsfMain
  kEsfPwrMgrElogErrorId0x40EsfMain = 0x40,
  kEsfPwrMgrElogErrorId0x41EsfMain,
  // PlWdt
  kEsfPwrMgrElogErrorId0x50PlWdt = 0x50,
  kEsfPwrMgrElogErrorId0x51PlWdt,
  kEsfPwrMgrElogErrorId0x52PlWdt,
  kEsfPwrMgrElogErrorId0x53PlWdt,
  kEsfPwrMgrElogErrorId0x54PlWdt,
  kEsfPwrMgrElogErrorId0x55PlWdt,
  kEsfPwrMgrElogErrorId0x56PlWdt,
  kEsfPwrMgrElogErrorId0x57PlWdt,
  kEsfPwrMgrElogErrorId0x58PlWdt,
  // PlPowerMgr
  kEsfPwrMgrElogErrorId0x60PlPowerMgr = 0x60,
  kEsfPwrMgrElogErrorId0x61PlPowerMgr,
  // HalGetVoltage
  kEsfPwrMgrElogErrorId0x70HalGetVoltage = 0x70,
  // PlSystemCtl
  kEsfPwrMgrElogErrorId0xc0PlSystemCtl = 0xc0,
  kEsfPwrMgrElogErrorId0xc1PlSystemCtl,
  kEsfPwrMgrElogErrorId0xc2PlSystemCtl,
  kEsfPwrMgrElogErrorId0xc3PlSystemCtl,
// Warning Id
  // EsfPwrMgr
  kEsfPwrMgrElogWarningId0x80EsfPwrMgr = 0x80,
  kEsfPwrMgrElogWarningId0x81EsfPwrMgr,
  kEsfPwrMgrElogWarningId0x82EsfPwrMgr,
  kEsfPwrMgrElogWarningId0x83EsfPwrMgr,
  // PlPowerMgr
  kEsfPwrMgrElogWarningId0x90PlPowerMgr = 0x90,
  // PlSystemCtl
  kEsfPwrMgrElogWarningId0xa0PlSystemCtl = 0xa0,
  kEsfPwrMgrElogWarningId0xa1PlSystemCtl,
  // EsfLedMgr
  kEsfPwrMgrElogWarningId0xb0EsfLedMgr = 0xb0,
} EsfFwMgrElogId;

#endif  // ESF_POWER_MANAGER_POWER_MANAGER_INTERNAL_H_
