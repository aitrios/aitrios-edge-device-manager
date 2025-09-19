/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <errno.h>

#include "pl_usb_port_controller.h"
#include "pl_usb_port_controller_os_impl.h"

// -----------------------------------------------------------------------------
PlErrCode PlUsbPCRegEventCallback(uint32_t usb_pc_id,
                                  PlUsbPCEventCallback callback,
                                  void *private_data) {
  return PlUsbPCRegEventCallbackOsImpl(usb_pc_id, callback, private_data);
}

// -----------------------------------------------------------------------------
PlErrCode PlUsbPCUnregEventCallback(uint32_t usb_pc_id) {
  return PlUsbPCUnregEventCallbackOsImpl(usb_pc_id);
}

// -----------------------------------------------------------------------------
PlErrCode PlUsbPCInitialize(void) {
  return PlUsbPCInitializeOsImpl();
}
// -----------------------------------------------------------------------------
PlErrCode PlUsbPCFinalize(void) {
  return PlUsbPCFinalizeOsImpl();
}
