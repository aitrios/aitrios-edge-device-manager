/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "clock_manager_internal.h"

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clock_manager_notification.h"
#include "clock_manager_setting_internal.h"
#include "clock_manager_utility.h"
#include "pl_clock_manager.h"
#include "utility_log.h"
#include "utility_log_module_id.h"

#ifndef CLOCK_MANAGER_REMOVE_STATIC
#define STATIC static
#else  // CLOCK_MANAGER_REMOVE_STATIC
#define STATIC
#endif  // CLOCK_MANAGER_REMOVE_STATIC

#ifndef INVALID_PROCESS_ID
#define INVALID_PROCESS_ID ((pid_t)(-1))
#endif

EsfClockManagerReturnValue EsfClockManagerWaitForTerminatingDaemon(void) {
  PlClockManagerReturnValue result = PlClockManagerWaitForTerminatingDaemon();
  if ((result == kPlClockManagerSuccess) ||
      (result == kPlClockManagerNotifierHasAlreadyFinished)) {
    return kClockManagerSuccess;
  } else {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- PlClockManagerWaitForTerminatingDaemon "
                     "failed:%d\n",
                     "clock_manager_internal.c", __LINE__, __func__, result);
    return kClockManagerStateTransitionError;
  }
}

EsfClockManagerReturnValue EsfClockManagerStartNtpClientDaemon(
    const EsfClockManagerParams *esf_param) {
  PlClockManagerParams *pl_params =
      (PlClockManagerParams *)malloc(sizeof(*pl_params));
  if (pl_params == NULL) {
    WRITE_DLOG_CRITICAL(MODULE_ID_SYSTEM,
                        "%s-%d:%s --- malloc(sizeof(*pl_params)) failed.\n",
                        "clock_manager_internal.c", __LINE__, __func__);
    return kClockManagerInternalError;
  }

  {
    // copy parameters from ESF to PL
    memcpy(pl_params->connect.hostname, esf_param->connect.hostname,
           sizeof(pl_params->connect.hostname));
    memcpy(pl_params->connect.hostname2, esf_param->connect.hostname2,
           sizeof(pl_params->connect.hostname2));

    pl_params->common.sync_interval = esf_param->common.sync_interval;
    pl_params->common.polling_time = esf_param->common.polling_time;

    pl_params->skip_and_limit.type =
        (PlClockManagerParamType)esf_param->skip_and_limit.type;
    pl_params->skip_and_limit.limit_packet_time =
        esf_param->skip_and_limit.limit_packet_time;
    pl_params->skip_and_limit.limit_rtc_correction_value =
        esf_param->skip_and_limit.limit_rtc_correction_value;
    pl_params->skip_and_limit.sanity_limit =
        esf_param->skip_and_limit.sanity_limit;

    pl_params->slew_setting.type =
        (PlClockManagerParamType)esf_param->slew_setting.type;
    pl_params->slew_setting.stable_rtc_correction_value =
        esf_param->slew_setting.stable_rtc_correction_value;
    pl_params->slew_setting.stable_sync_number =
        esf_param->slew_setting.stable_sync_number;
  }

  PlClockManagerReturnValue pl_rv =
      PlClockManagerStartNtpClientDaemon(pl_params);

  free(pl_params);

  if (pl_rv != kPlClockManagerSuccess) {
    WRITE_DLOG_ERROR(MODULE_ID_SYSTEM,
                     "%s-%d:%s --- PlClockManagerStartNtpClientDaemon "
                     "failed:%d\n",
                     "clock_manager_internal.c", __LINE__, __func__, pl_rv);
    return kClockManagerInternalError;
  }

  return kClockManagerSuccess;
}
