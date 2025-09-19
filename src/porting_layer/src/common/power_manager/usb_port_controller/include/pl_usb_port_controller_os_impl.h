/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef __PL_USB_PORT_CTRL_OS_IMPL_H__
#define __PL_USB_PORT_CTRL_OS_IMPL_H__

#include "pl.h"
#include "pl_usb_port_controller.h"

PlErrCode PlUsbPCInitializeOsImpl(void);
PlErrCode PlUsbPCFinalizeOsImpl(void);
PlErrCode PlUsbPCRegEventCallbackOsImpl(uint32_t usb_pc_id,
                                        PlUsbPCEventCallback callback,
                                        void *private_data);

PlErrCode PlUsbPCUnregEventCallbackOsImpl(uint32_t usb_pc_id);

#endif  // _PL_USB_PORT_CTRL_OS_IMPL_H_
