/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Includes --------------------------------------------------------------------
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <inttypes.h>

#ifdef __NuttX__
#include <sched/sched.h>
#include <nuttx/board.h>
#include <nuttx/arch.h>
#include <nuttx/sched.h>
#endif // __NuttX__

#include "pl_system_control.h"
#include "pl_system_control_impl.h"
#ifdef CONFIG_EXTERNAL_PL_WDT
#include "pl_wdt_lib.h"
#endif

#ifdef __NuttX__
#include "board_crashdump.h"
#endif // __NuttX__

#include "utility_log_module_id.h"
#include "utility_log.h"
// Macros ----------------------------------------------------------------------
#define EVENT_ID  0x9800
#define EVENT_ID_START 0x0000
#define LOG_ERR(event_id, format, ...) \
  WRITE_DLOG_ERROR(MODULE_ID_SYSTEM, \
    "%s-%d:" format, __FILE__, __LINE__, ##__VA_ARGS__); \
  WRITE_ELOG_ERROR(MODULE_ID_SYSTEM, (EVENT_ID | \
                  (0x00FF & (EVENT_ID_START + event_id))));

#define BIT32_PID_MAX (32768)

// Typedefs --------------------------------------------------------------------

// External functions ----------------------------------------------------------

// Local functions -------------------------------------------------------------
#ifdef __NuttX__
static int SafeSnprintf(char *dst, size_t size, const char *fmt, ...);
static void UpPuts(const char *format, ...);
static void DumpRegs(uint32_t *regs);
static void DumpStack(uint32_t stack_end, uint32_t stack_top);
#endif // __NuttX__

// Global Variables ------------------------------------------------------------
static bool s_is_initialized = false;
static pthread_mutex_t s_initialize_mutex = PTHREAD_MUTEX_INITIALIZER;
#ifdef __NuttX__
static pthread_mutex_t s_exception_mutex  = PTHREAD_MUTEX_INITIALIZER;
#endif // __NuttX__

// Functions -------------------------------------------------------------------

//------------------------------------------------------------------------------
//    PlSystemCtlExec
//------------------------------------------------------------------------------
PlErrCode PlSystemCtlExecOperation(PlSystemCtlOperation operation) {
  // In the case of reboot, whether PllInitialize() has run or not is ignored.
  PlErrCode ret_code = kPlErrCodeOk;
  int lock_ret = pthread_mutex_lock(&s_initialize_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x00, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  if (((operation != kPlSystemCtlRebootCpu) &&
       (operation != kPlSystemCtlRebootEdgeDevice)) &&
      (!s_is_initialized)) {
    LOG_ERR(0x01, "State error. operation=%d, s_is_initialized=%d",
          operation, s_is_initialized);
    ret_code = kPlErrInvalidState;
    goto unlock;
  }

  switch (operation) {
    case kPlSystemCtlRebootCpu:
      ret_code = PlSystemCtlRebootCpuImpl();
      goto unlock;
    case kPlSystemCtlRebootEdgeDevice:
      ret_code = PlSystemCtlRebootEdgeDeviceImpl();
      goto unlock;
    case kPlSystemCtlPowerOff:
      // T.B.D.
    default:
      LOG_ERR(0x03, "Parameter error(%d).", operation);
      ret_code = kPlErrInvalidParam;
      goto unlock;
  }

unlock:
  lock_ret = pthread_mutex_unlock(&s_initialize_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x04, "Unlock error. errno=%d\n", errno);
  }

  return ret_code;
}

//------------------------------------------------------------------------------
//    PlSystemCtlGetResetCause
//------------------------------------------------------------------------------
PlErrCode PlSystemCtlGetResetCause(PlSystemCtlResetCause *cause) {
#ifdef __NuttX__
  if (cause == NULL) {
    LOG_ERR(0x05, "Parameter error.");
    return kPlErrInvalidParam;
  }

  return PlSystemCtlGetResetCauseImpl(cause);
#else
  return kPlErrNoSupported;
#endif // __NuttX__
}

//------------------------------------------------------------------------------
//    PlSystemCtlGetExceptionInfo
//------------------------------------------------------------------------------
#ifdef __NuttX__
PlErrCode PlSystemCtlGetExceptionInfo(
                                  struct PlSystemCtlExceptionInfo *info) {
  int lock_ret = pthread_mutex_lock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x06, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  PlErrCode ret_code = kPlErrCodeOk;
  if (info == NULL) {
    LOG_ERR(0x07, "Parameter error.");
    ret_code = kPlErrInvalidParam;
    goto unlock;
  }
  char *magiccode = PlSystemCtlGetRtcAddrImpl();   // RTC RAM
  uint32_t str_len = strlen(EXCEPTION_INFO_MAGICCODE);
  if (strncmp(magiccode, EXCEPTION_INFO_MAGICCODE, str_len) != 0) {
    LOG_ERR(0x08, "MagicCode error.");
    ret_code = kPlErrMagicCode;
    goto unlock;
  }

  struct PlSystemCtlExceptionInfo *exp_info =
          (struct PlSystemCtlExceptionInfo *)(PlSystemCtlGetRtcAddrImpl() +
                                              EXCEPTION_INFO_MAGICCODE_LEN);

  memcpy(info, exp_info, sizeof(struct PlSystemCtlExceptionInfo));

unlock:
  lock_ret = pthread_mutex_unlock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x0F, "Unlock error. errno=%d\n", errno);
  }

  return ret_code;
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    PlSystemCtlSetExceptionInfo
//------------------------------------------------------------------------------
#ifdef __NuttX__
PlErrCode PlSystemCtlSetExceptionInfo(void) {
  int lock_ret = pthread_mutex_lock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x10, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  FAR struct tcb_s *rtcb = running_task();
  board_crashdump((uintptr_t)0x0, rtcb, NULL, 0, NULL, NULL);

  lock_ret = pthread_mutex_unlock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x11, "Unlock error. errno=%d\n", errno);
  }

  return kPlErrCodeOk;
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    PlSystemCtlConvExceptionInfo
//------------------------------------------------------------------------------
#ifdef __NuttX__
PlErrCode PlSystemCtlConvExceptionInfo(
                                  struct PlSystemCtlExceptionInfo *info,
                                  char *dst, uint32_t dst_size) {
  int lock_ret = pthread_mutex_lock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x18, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  PlErrCode ret_code = kPlErrCodeOk;
  if ((info == NULL) || (dst == NULL) || (dst_size == 0)) {
    LOG_ERR(0x19, "Parameter error.");
    ret_code = kPlErrInvalidParam;
    goto unlock;
  }
  if (dst_size == 1) {
    dst[0] = '\0';
    ret_code = kPlErrCodeOk;  // return empty string.
    goto unlock;
  }

  uint32_t idx = 0;

  info->taskname[sizeof(info->taskname) - 1] = '\0';
  int write_size =
        SafeSnprintf(&(dst[idx]), dst_size - idx - 1,  // -1: NULL char
                     "Exception: task: %s\n", info->taskname);
  if (write_size < 0) {
    LOG_ERR(0x1A, "snprintf error. (ret=%d)", write_size);
    ret_code = kPlErrWrite;
    goto unlock;
  }
  idx += write_size;
  if ((idx + 1) == dst_size) {  // +1: NULL char
    ret_code = kPlErrCodeOk;  // full write done.
    goto unlock;
  }

  write_size = SafeSnprintf(&(dst[idx]), dst_size - idx - 1,  // -1: NULL char
                            "    PC: %08" PRIx32 "\n", info->reg_pc);
  if (write_size < 0) {
    LOG_ERR(0x1B, "snprintf error. (ret=%d)", write_size);
    ret_code = kPlErrWrite;
    goto unlock;
  }
  idx += write_size;
  if ((idx + 1) == dst_size) {  // +1: NULL char
    ret_code = kPlErrCodeOk;  // full write done.
    goto unlock;
  }

  write_size = SafeSnprintf(&(dst[idx]), dst_size - idx - 1,  // -1: NULL char
                            "  base: %08" PRIx32 "\n", info->stack_base);
  if (write_size < 0) {
    LOG_ERR(0x1C, "snprintf error. (ret=%d)", write_size);
    ret_code = kPlErrWrite;
    goto unlock;
  }
  idx += write_size;
  if ((idx + 1) == dst_size) {  // +1: NULL char
    ret_code = kPlErrCodeOk;  // full write done.
    goto unlock;
  }

  write_size = SafeSnprintf(&(dst[idx]), dst_size - idx - 1,  // -1: NULL char
                            "  size: %08" PRIx32 "\n", info->stack_size);
  if (write_size < 0) {
    LOG_ERR(0x1D, "snprintf error. (ret=%d)", write_size);
    ret_code = kPlErrWrite;
    goto unlock;
  }
  idx += write_size;
  if ((idx + 1) == dst_size) {  // +1: NULL char
    ret_code = kPlErrCodeOk;  // full write done.
    goto unlock;
  }

#ifdef CONFIG_STACK_COLORATION
  write_size = SafeSnprintf(&(dst[idx]), dst_size - idx - 1,  // -1: NULL char
                            "  used: %08" PRIx32 "\n", info->stack_used);
  if (write_size < 0) {
    LOG_ERR(0x1E, "snprintf error. (ret=%d)", write_size);
    ret_code = kPlErrWrite;
    goto unlock;
  }
  idx += write_size;
  if ((idx + 1) == dst_size) {  // +1: NULL char
    ret_code = kPlErrCodeOk;  // full write done.
    goto unlock;
  }
#endif

  if (EXCEPTION_INFO_STACKDUMP_NUM < info->stack_num) {
    info->stack_num = EXCEPTION_INFO_STACKDUMP_NUM;
  }

  for (uint32_t num = 0; num < info->stack_num; num++) {
    write_size = SafeSnprintf(
                  &(dst[idx]), dst_size - idx - 1,   // -1: NULL char
                  "%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
                  info->stack_dump[num].addr,
                  info->stack_dump[num].data[0], info->stack_dump[num].data[1],
                  info->stack_dump[num].data[2], info->stack_dump[num].data[3],
                  info->stack_dump[num].data[4], info->stack_dump[num].data[5],
                  info->stack_dump[num].data[6], info->stack_dump[num].data[7]);
    if (write_size < 0) {
      LOG_ERR(0x1F, "snprintf error. (ret=%d)", write_size);
      ret_code = kPlErrWrite;
      goto unlock;
    }
    idx += write_size;
    if ((idx + 1) == dst_size) {  // +1: NULL char
      ret_code = kPlErrCodeOk;  // full write done.
      goto unlock;
    }
  }

unlock:
  lock_ret = pthread_mutex_unlock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x20, "Unlock error. errno=%d\n", errno);
  }

  return ret_code;
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    PlSystemCtlClearExceptionInfo
//------------------------------------------------------------------------------
#ifdef __NuttX__
PlErrCode PlSystemCtlClearExceptionInfo(void) {
  int lock_ret = pthread_mutex_lock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x21, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }
  char *rtc_addr = PlSystemCtlGetRtcAddrImpl();   // RTC RAM
  memset(rtc_addr, 0, EXCEPTION_INFO_MAGICCODE_LEN +
                      sizeof(struct PlSystemCtlExceptionInfo));

  lock_ret = pthread_mutex_unlock(&s_exception_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x22, "Unlock error. errno=%d\n", errno);
  }

  return kPlErrCodeOk;
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    PlSystemCtlDumpAllStack
//------------------------------------------------------------------------------
#ifdef __NuttX__
PlErrCode PlSystemCtlDumpAllStack(void) {
  struct tcb_s *rtcb = this_task();
  if (rtcb->xcp.regs != NULL) {
    UpPuts("Regs:\n");
    UpPuts("  xcp.regs:%p\n", rtcb->xcp.regs);
    DumpRegs(rtcb->xcp.regs);
  }
  if (rtcb->xcp.saved_regs != NULL) {
    UpPuts("Saved Regs:\n");
    UpPuts("  xcp.saved_regs:%p\n", rtcb->xcp.saved_regs);
    DumpRegs(rtcb->xcp.saved_regs);
  }

  UpPuts("sp reg: %08lx\n", up_getsp());

  uint32_t ustackbase = (uint32_t)rtcb->stack_base_ptr;
  uint32_t ustacksize = (uint32_t)rtcb->adj_stack_size;
  UpPuts("User stack:\n");
  UpPuts("  base(Top): %08x\n", ustackbase + ustacksize);
  UpPuts("  size: %08x\n", ustacksize);

  UpPuts("current pid=%d\n", pthread_self());
  for (int pid=0; pid < BIT32_PID_MAX; pid++) {
    struct tcb_s *tcb = nxsched_get_tcb(pid);
    if (tcb) {
      UpPuts("pid:%d %s\n", pid, tcb->name);
      if (tcb->xcp.regs != NULL) {
        UpPuts("Regs:\n");
        UpPuts("  xcp.regs:%p\n", tcb->xcp.regs);
        DumpRegs(tcb->xcp.regs);
      }
      if (tcb->xcp.saved_regs != NULL) {
        UpPuts("Saved Regs:\n");
        UpPuts("  xcp.saved_regs:%p\n", tcb->xcp.saved_regs);
        DumpRegs(tcb->xcp.saved_regs);
      }

      ustackbase = (uint32_t)tcb->stack_base_ptr;
      ustacksize = (uint32_t)tcb->adj_stack_size;

      UpPuts("User stack:\n");
      UpPuts("  base(Top): %08x\n", ustackbase + ustacksize);
      UpPuts("  size: %08x\n", ustacksize);

      DumpStack(ustackbase, ustackbase + ustacksize);
      UpPuts("---------------------------------------------\n");
      const int wdt_no = 1;
      PlErrCode pl_ret = PlWdtLibKeepAliveIrqContext(wdt_no);
      if (pl_ret != kPlErrCodeOk) {
        UpPuts("Failed to PlWdtLibKeepAlive:%d\n", pl_ret);
        return pl_ret;
      }
    }
  }

  return kPlErrCodeOk;
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    PlSystemCtlInitialize
//------------------------------------------------------------------------------
PlErrCode PlSystemCtlInitialize(void) {
  int lock_ret = pthread_mutex_lock(&s_initialize_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x14, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  // nothing to do.
  s_is_initialized = true;

  lock_ret = pthread_mutex_unlock(&s_initialize_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x15, "Unlock error. errno=%d\n", errno);
  }

  return kPlErrCodeOk;
}

//------------------------------------------------------------------------------
//    PlSystemCtlFinalize
//------------------------------------------------------------------------------
PlErrCode PlSystemCtlFinalize(void) {
  int lock_ret = pthread_mutex_lock(&s_initialize_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x16, "Lock error. errno=%d\n", errno);
    return kPlErrLock;
  }

  // nothing to do.
  s_is_initialized = false;

  lock_ret = pthread_mutex_unlock(&s_initialize_mutex);
  if (lock_ret != 0) {
    LOG_ERR(0x17, "Unlock error. errno=%d\n", errno);
  }

  return kPlErrCodeOk;
}

//------------------------------------------------------------------------------
//    SafeSnprintf
//------------------------------------------------------------------------------
#ifdef __NuttX__
static int SafeSnprintf(char *dst, size_t size, const char *fmt, ...) {
  va_list list = {0};
  char src[255] = {0};

  va_start(list, fmt);
  vsnprintf(src, sizeof(src), fmt, list);
  va_end(list);

  int src_size = strlen(src);
  int write_size = (int)size < src_size ? (int)size : src_size;
  memcpy(dst, src, write_size);
  dst[write_size] = '\0';
  return write_size;
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    UpPuts
//------------------------------------------------------------------------------
#ifdef __NuttX__
static void UpPuts(const char *format, ...) {
  va_list list = {0};
  char buf[128] = {0};
  va_start(list, format);
  vsnprintf(buf, sizeof(buf) - 1, format, list);
  va_end(list);
  up_puts(buf);
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    DumpRegs
//------------------------------------------------------------------------------
#ifdef __NuttX__
static void DumpRegs(uint32_t *regs) {
  UpPuts("   PC: %08lx    PS: %08lx\n",
      (unsigned long)regs[REG_PC], (unsigned long)regs[REG_PS]);  //NOLINT
  UpPuts("   A0: %08lx    A1: %08lx    A2: %08lx    A3: %08lx\n",
      (unsigned long)regs[REG_A0], (unsigned long)regs[REG_A1],  //NOLINT
      (unsigned long)regs[REG_A2], (unsigned long)regs[REG_A3]);  //NOLINT
  UpPuts("   A4: %08lx    A5: %08lx    A6: %08lx    A7: %08lx\n",
      (unsigned long)regs[REG_A4], (unsigned long)regs[REG_A5],  //NOLINT
      (unsigned long)regs[REG_A6], (unsigned long)regs[REG_A7]);  //NOLINT
  UpPuts("   A8: %08lx    A9: %08lx   A10: %08lx   A11: %08lx\n",
      (unsigned long)regs[REG_A8], (unsigned long)regs[REG_A9],  //NOLINT
      (unsigned long)regs[REG_A10], (unsigned long)regs[REG_A11]);  //NOLINT
  UpPuts("  A12: %08lx   A13: %08lx   A14: %08lx   A15: %08lx\n",
      (unsigned long)regs[REG_A12], (unsigned long)regs[REG_A13],  //NOLINT
      (unsigned long)regs[REG_A14], (unsigned long)regs[REG_A15]);  //NOLINT
  UpPuts("  SAR: %08lx CAUSE: %08lx VADDR: %08lx\n",
      (unsigned long)regs[REG_SAR],(unsigned long)regs[REG_EXCCAUSE],  //NOLINT
      (unsigned long)regs[REG_EXCVADDR]);  //NOLINT
}
#endif // __NuttX__

//------------------------------------------------------------------------------
//    DumpStack
//------------------------------------------------------------------------------
#ifdef __NuttX__
static void DumpStack(uint32_t stack_end, uint32_t stack_top) {
    for (uint32_t stack = stack_top & ~0x1f; stack >= stack_end; stack -= 32) {
      uint32_t *ptr = (uint32_t *)stack;
    UpPuts("%08x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
                stack, ptr[0], ptr[1], ptr[2], ptr[3],
                ptr[4], ptr[5], ptr[6], ptr[7]);
    }
}
#endif // __NuttX__
