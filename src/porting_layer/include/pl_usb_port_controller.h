/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef __PL_USB_PORT_CTRL_H__
#define __PL_USB_PORT_CTRL_H__

#include "pl.h"

typedef enum {
  kPlUsbPCEventConnected,
  kPlUsbPCEventDisconnected,
  kPlUsbPCEventMax
} PlUsbPCEvent;

typedef void (*PlUsbPCEventCallback)(PlUsbPCEvent event, void *private_data);

PlErrCode PlUsbPCInitialize(void);
PlErrCode PlUsbPCFinalize(void);
PlErrCode PlUsbPCRegEventCallback(uint32_t usb_pc_id,
                                    PlUsbPCEventCallback callback,
                                    void *private_data);

PlErrCode PlUsbPCUnregEventCallback(uint32_t usb_pc_id);

#endif  // _PL_USB_PORT_CTRL_H_
