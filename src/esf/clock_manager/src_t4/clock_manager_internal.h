/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESF_CLOCK_MANAGER_INTERNAL_H_
#define ESF_CLOCK_MANAGER_INTERNAL_H_

#include <pthread.h>
#include <stdint.h>

#include "clock_manager.h"
#include "clock_manager_setting.h"
#include "clock_manager_utility.h"

EsfClockManagerReturnValue EsfClockManagerRegisterNtpSyncCompleteCb(
    void (*on_ntp_sync_complete)(bool));

EsfClockManagerReturnValue EsfClockManagerCreateNotifierThread(
    EsfClockManagerMillisecondT surveillance_period);

EsfClockManagerReturnValue EsfClockManagerDestroyNotifier(void);


EsfClockManagerReturnValue EsfClockManagerWaitForTerminatingDaemon(void);

EsfClockManagerReturnValue EsfClockManagerStartNtpClientDaemon(
    const EsfClockManagerParams *esf_param);

#endif  // ESF_CLOCK_MANAGER_INTERNAL_H_
