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
#include "pl_usb_port_controller_cam_impl.h"

// -----------------------------------------------------------------------------
PlErrCode PlUsbPCRegEventCallbackOsImpl(uint32_t usb_pc_id,
                                        PlUsbPCEventCallback callback,
                                        void *private_data) {
  (void)usb_pc_id;       // Avoid compiler warning
  (void)callback;        // Avoid compiler warning
  (void)private_data;    // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlUsbPCUnregEventCallbackOsImpl(uint32_t usb_pc_id) {
  (void)usb_pc_id;   // Avoid compiler warning
  return kPlErrNoSupported;
}

// -----------------------------------------------------------------------------
PlErrCode PlUsbPCInitializeOsImpl(void) {
  return kPlErrNoSupported;
}
// -----------------------------------------------------------------------------
PlErrCode PlUsbPCFinalizeOsImpl(void) {
  return kPlErrNoSupported;
}
