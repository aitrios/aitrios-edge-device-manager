/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <nuttx/board.h>
#include <nuttx/irq.h>
#include <xtensa.h>

#include "pl_system_control.h"
#include "pl_system_control_impl.h"
#include "board_crashdump.h"

#include "utility_log_module_id.h"
#include "utility_log.h"

// Macros ----------------------------------------------------------------------
#define EVENT_ID  0x9800
#define EVENT_ID_START 0x0018
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                  (0x00FF & (EVENT_ID_START + event_id))));

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------

// Global Variables ------------------------------------------------------------

// Functions -------------------------------------------------------------------
void board_crashdump(uintptr_t sp, struct tcb_s *tcb,
                     const char *filename, int lineno,
                     const char *msg, void *regs) {
  (void)sp;         // Avoid compiler warning
  (void)filename;   // Avoid compiler warning
  (void)lineno;     // Avoid compiler warning
  (void)msg;        // Avoid compiler warning
  (void)regs;       // Avoid compiler warning
  char *magiccode = (char *)PlSystemCtlGetRtcAddrImpl();  // RTC RAM
  struct PlSystemCtlExceptionInfo *dump_body =
          (struct PlSystemCtlExceptionInfo *)(PlSystemCtlGetRtcAddrImpl() +
          EXCEPTION_INFO_MAGICCODE_LEN);

  /* Clear Exception Information */
  memset(magiccode, 0, EXCEPTION_INFO_MAGICCODE_LEN);
  memset(dump_body, 0, sizeof(struct PlSystemCtlExceptionInfo));

  /* Store MagicCode */
  strncpy(magiccode, EXCEPTION_INFO_MAGICCODE, EXCEPTION_INFO_MAGICCODE_LEN);

  /* Store Exception Information(taskname) -1: NULL char */
  strncpy(dump_body->taskname, tcb->name, sizeof(dump_body->taskname) - 1);

  /* Store Exception Information(PC) */
  if (CURRENT_REGS != NULL) {
    dump_body->reg_pc = CURRENT_REGS[REG_PC];
  } else if (tcb->xcp.saved_regs != NULL) {
    dump_body->reg_pc = tcb->xcp.saved_regs[REG_PC];
  } else if (tcb->xcp.regs != NULL) {
    dump_body->reg_pc = tcb->xcp.regs[REG_PC];
  } else {
    LOG_ERR(0x00, "Not found PC register.\n");
    return;
  }
  /* Store Exception Information(stack info) */
  dump_body->stack_base = (uint32_t)tcb->stack_base_ptr;
  dump_body->stack_size = (uint32_t)tcb->adj_stack_size;

  uint32_t base = dump_body->stack_base;
  uint32_t size = dump_body->stack_size;
#ifdef CONFIG_STACK_COLORATION
  dump_body->stack_used = xtensa_stack_check(base, size);
#endif

  /* Store Exception Information(stack addrs) */
  uint32_t stack_top = base + size;
  dump_body->stack_num = 0;
  for (uint32_t stack_addr = base & ~0x1f; stack_addr < (stack_top & ~0x1f);
       stack_addr += 32) {
    uintptr_t ptr = (uintptr_t)stack_addr;
    if (dump_body->stack_num < EXCEPTION_INFO_STACKDUMP_NUM) {
      memcpy(dump_body->stack_dump[dump_body->stack_num].data, (void *)ptr,
                  (sizeof(uint32_t) * EXCEPTION_INFO_STACKDUMP_DATA_NUM));
      dump_body->stack_dump[dump_body->stack_num].addr = stack_addr;
      dump_body->stack_num++;
    }
  }
  return;
}
