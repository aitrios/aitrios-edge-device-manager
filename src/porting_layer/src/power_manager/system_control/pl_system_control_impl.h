/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_SYSTEM_CONTROL_IMPL_H
#define __PL_SYSTEM_CONTROL_IMPL_H

#include <stdint.h>
#include "pl.h"

// Functions ------------------------------------------------------------------
PlErrCode PlSystemCtlGetResetCauseImpl(PlSystemCtlResetCause *cause);
char* PlSystemCtlGetRtcAddrImpl(void);
PlErrCode PlSystemCtlRebootEdgeDeviceImpl(void);

#endif /* __PL_SYSTEM_CONTROL_IMPL_H */
