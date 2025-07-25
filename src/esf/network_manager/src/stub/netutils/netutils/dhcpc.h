/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APPS_INCLUDE_NETUTILS_DHCPC_H
#define __APPS_INCLUDE_NETUTILS_DHCPC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <netinet/in.h>
#ifndef __NuttX__
#define FAR
#endif  // __NuttX__

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

struct dhcpc_state {
  struct in_addr serverid;
  struct in_addr ipaddr;
  struct in_addr netmask;
  struct in_addr dnsaddr;
  struct in_addr default_router;
  uint32_t       lease_time;      /* Lease expires in this number of seconds */
  uint32_t       renewal_time;    /* Seconds to transition to RENEW state(T1) */
  uint32_t       rebinding_time;  /* Seconds to transition to REBIND state(T2) */
};

typedef void (*dhcpc_callback_t)(FAR struct dhcpc_state *presult);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

FAR void *dhcpc_open(FAR const char *interface,
                     FAR const void *mac_addr, int mac_len);
int  dhcpc_request(FAR void *handle, FAR struct dhcpc_state *presult);
int  dhcpc_request_async(FAR void *handle, dhcpc_callback_t callback);
int  dhcpc_renew(FAR void *handle, FAR struct dhcpc_state *presult);
void dhcpc_cancel(FAR void *handle);
void dhcpc_close(FAR void *handle);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_DHCPC_H */
