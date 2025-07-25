/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_IMPL_H_
#define PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_IMPL_H_

// -----------------------------------------------------------------------------
#ifdef __NuttX__
#include <sys/queue.h>
#include <sched.h>

inline static irqstate_t NetworkLock(void) {
  irqstate_t flags = 0;
#ifdef CONFIG_SMP
  flags = spin_lock_irqsave(&s_spin_lock);
#else
  sched_lock();
#endif
  return flags;
}

inline static void NetworkUnlock(irqstate_t flags) {
#ifdef CONFIG_SMP
  spin_unlock_irqrestore(&s_spin_lock, flags);
#else
  (void)flags;
  sched_unlock();
#endif
  return;
}
#endif  // #ifdef __NuttX__

// -----------------------------------------------------------------------------
#ifndef __NuttX__
#include <bsd/sys/queue.h>
#include <pthread.h>

typedef int irqstate_t;
static pthread_mutex_t s_pl_tailq_mutex = PTHREAD_MUTEX_INITIALIZER;

inline static irqstate_t NetworkLock(void) {
  return pthread_mutex_lock(&s_pl_tailq_mutex);
}

inline static void NetworkUnlock(irqstate_t flags) {
  (void)flags;
  pthread_mutex_unlock(&s_pl_tailq_mutex);
  return;
}
#endif  // #ifndef __NuttX__

#endif  // PORTING_LAYER_SRC_NETWORK_MANAGER_NETWORK_IMPL_H_
