/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef __BOARD_CRASHDUMP_H__
#define __BOARD_CRASHDUMP_H__

#include <stdint.h>
#include "pl_system_control.h"

// Macros ----------------------------------------------------------------------
#define EXCEPTION_INFO_MAGICCODE            ("WDT reboot")
#define EXCEPTION_INFO_MAGICCODE_LEN        (16)
#define EXCEPTION_INFO_STACKDUMP_NUM \
          (PL_SYSTEMCTL_EXCEPTION_INFO_STACKDUMP_NUM)
#define EXCEPTION_INFO_STACKDUMP_DATA_NUM  \
          (PL_SYSTEMCTL_EXCEPTION_INFO_STACKDUMP_DATA_NUM)

// Typedefs --------------------------------------------------------------------

#endif  // __BOARD_CRASHDUMP_H__
