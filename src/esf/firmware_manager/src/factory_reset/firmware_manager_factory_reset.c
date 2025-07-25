/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "firmware_manager_factory_reset.h"

#include <stddef.h>

#include "firmware_manager.h"
#include "firmware_manager_log.h"
#include "led_manager.h"
#include "main.h"

// Internal functions ##########################################################

static void SetLedStateForFactoryResetting(void) {
  EsfLedManagerLedStatusInfo status;
  status.enabled = true;
  status.led = kEsfLedManagerTargetLedPower;
  status.status = kEsfLedManagerLedStatusResetting;
  EsfLedManagerResult ret = EsfLedManagerSetStatus(&status);
  if (ret != kEsfLedManagerSuccess) {
    ESF_FW_MGR_DLOG_CRITICAL("EsfLedManagerSetStatus failed. ret = %u\n", ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xf0FactoryReset);
  }
}

// Public functions
// ############################################################

EsfFwMgrResult EsfFwMgrStartFactoryResetInternal(
    EsfFwMgrFactoryResetCause cause) {
  EsfMainMsgType message = kEsfMainMsgTypeFactoryReset;

  if (cause == kEsfFwMgrResetCauseDowngrade)
    message = kEsfMainMsgTypeFactoryResetForDowngrade;

  EsfMainError main_ret = EsfMainNotifyMsg(message);
  if (main_ret != kEsfMainOk) {
    ESF_FW_MGR_DLOG_CRITICAL("EsfMainNotifyMsg failed. ret = %u.\n", main_ret);
    ESF_FW_MGR_ELOG_CRITICAL(kEsfFwMgrElogCriticalId0xf1FactoryReset);
    return kEsfFwMgrResultUnavailable;
  }

  SetLedStateForFactoryResetting();

  return kEsfFwMgrResultOk;
}
