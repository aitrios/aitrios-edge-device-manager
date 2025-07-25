/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "memory_manager_export_api.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "memory_manager.h"

/*
 * Note: this allows any wasm instances importing this function
 * to access any esf memory handles compatible with Fopen on the
 * system. Maybe it's better to provide some kind of isolation.
 */

uint32_t EsfMemoryManagerPread_wasm(wasm_exec_env_t exec_env, uint32_t handle,
                                    void *buf, uint32_t sz, uint64_t offset,
                                    uint32_t *resultp) {
  wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
  EsfMemoryManagerResult ret;
  EsfMemoryManagerResult ret1;
  off_t roffset;

  /* Note: buf,sz should have already been validated by wamr */
  /* TODO: size_t and off_t range check */
  if (!wasm_runtime_validate_native_addr(inst, resultp, sizeof(uint32_t))) {
    /* REVISIT: is it more appropriate to raise a trap? */
    return kEsfMemoryManagerResultOtherError;
  }
  /*
   * REVISIT: Depending on how expensive Fopen/Fclose is, it might be
   * better to keep it open.
   */
  ret = EsfMemoryManagerFopen(handle);
  if (ret != kEsfMemoryManagerResultSuccess) {
    return ret;
  }
  /*
   * TODO: Use EsfMemoryManagerPread when available.
   */
  ret = EsfMemoryManagerFseek(handle, offset, SEEK_SET, &roffset);
  if (ret != kEsfMemoryManagerResultSuccess) {
    goto fail;
  }
  size_t rsz;
  ret = EsfMemoryManagerFread(handle, buf, sz, &rsz);
  if (ret != kEsfMemoryManagerResultSuccess) {
    goto fail;
  }
  uint32_t le32 = rsz; /* Note: little endian assumption */
  memcpy(resultp, &le32, sizeof(le32));
fail:
  ret1 = EsfMemoryManagerFclose(handle);
  if (ret1 != kEsfMemoryManagerResultSuccess) {
    /* no way to recover. TODO: log */
  }
  return ret;
}
