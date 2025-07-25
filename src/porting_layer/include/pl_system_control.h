/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __PL_SYSTEM_CONTROL_H__
#define __PL_SYSTEM_CONTROL_H__

#include <stdint.h>
#include "pl.h"

// Macros ----------------------------------------------------------------------
#ifdef CONFIG_STACK_COLORATION
#define PL_SYSTEMCTL_EXCEPTION_INFO_SIZE                (18158)
#else
#define PL_SYSTEMCTL_EXCEPTION_INFO_SIZE                (18141)
#endif
#define PL_SYSTEMCTL_EXCEPTION_INFO_STACKDUMP_NUM       (220)
#define PL_SYSTEMCTL_EXCEPTION_INFO_STACKDUMP_DATA_NUM  (8)

// Typedefs --------------------------------------------------------------------
struct PlSystemCtlExceptionInfoBody {
  uint32_t addr;
  uint32_t data[PL_SYSTEMCTL_EXCEPTION_INFO_STACKDUMP_DATA_NUM];
};

struct PlSystemCtlExceptionInfo {
  char          taskname[32];
  uint32_t      reg_pc;
  uint32_t      stack_base;
  uint32_t      stack_size;
#ifdef CONFIG_STACK_COLORATION
  uint32_t      stack_used;
#endif
  uint32_t      stack_num;
  struct PlSystemCtlExceptionInfoBody
                stack_dump[PL_SYSTEMCTL_EXCEPTION_INFO_STACKDUMP_NUM];
};

// Global Variables -----------------------------------------------------------
typedef enum {
  kPlSystemCtlRebootCpu = 0,
  kPlSystemCtlRebootEdgeDevice,
  kPlSystemCtlPowerOff,
  kPlSystemCtlMax
} PlSystemCtlOperation;

typedef enum {
  kPlSystemCtlResetCauseSysChipPowerOnReset = 0,
  kPlSystemCtlResetCauseSysBrownOut,
  kPlSystemCtlResetCauseCoreSoft,
  kPlSystemCtlResetCauseCoreDeepSleep,
  kPlSystemCtlResetCauseWDT,
  kPlSystemCtlResetCauseMax
} PlSystemCtlResetCause;

// Functions ------------------------------------------------------------------
PlErrCode PlSystemCtlInitialize(void);
PlErrCode PlSystemCtlFinalize(void);
PlErrCode PlSystemCtlExecOperation(PlSystemCtlOperation operation);
PlErrCode PlSystemCtlGetResetCause(PlSystemCtlResetCause *cause);
PlErrCode PlSystemCtlGetExceptionInfo(
            struct PlSystemCtlExceptionInfo *info);
PlErrCode PlSystemCtlSetExceptionInfo(void);
PlErrCode PlSystemCtlConvExceptionInfo(
            struct PlSystemCtlExceptionInfo *info,
            char *dst, uint32_t dst_size);
PlErrCode PlSystemCtlClearExceptionInfo(void);
PlErrCode PlSystemCtlDumpAllStack(void);

#endif /* __PL_SYSTEM_CONTROL_H__ */
