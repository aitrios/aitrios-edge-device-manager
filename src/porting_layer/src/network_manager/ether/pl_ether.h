/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_ETHER_H_
#define PL_ETHER_H_

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Interface Name
#ifdef CONFIG_PL_ETH0_IFNAME
# define  NETWORK_ETH0_IFNAME             CONFIG_PL_ETH0_IFNAME
#else  // CONFIG_PL_ETH0_IFNAME
# define  NETWORK_ETH0_IFNAME             "eth0"
#endif  // CONFIG_PL_ETH0_IFNAME

// Cloud Connection
#ifdef CONFIG_PL_ETH0_CLOUD
# define  NETWORK_ETH0_CLOUD              true
#else  // CONFIG_PL_ETH0_CLOUD
# define  NETWORK_ETH0_CLOUD              false
#endif  // CONFIG_PL_ETH0_CLOUD

// Local Connection
#ifdef CONFIG_PL_ETH0_LOCAL
# define  NETWORK_ETH0_LOCAL              true
#else  // CONFIG_PL_ETH0_LOCAL
# define  NETWORK_ETH0_LOCAL              false
#endif  // CONFIG_PL_ETH0_LOCAL

// Monitor Thread Stack Size
#ifdef CONFIG_PL_ETHER_MONITOR_STACKSIZE
# define ETHER_MONITOR_THREAD_STACKSIZE   CONFIG_PL_ETHER_MONITOR_STACKSIZE
#else  // CONFIG_PL_ETHER_MONITOR_STACKSIZE
# define ETHER_MONITOR_THREAD_STACKSIZE   (4 * 1024)
#endif  // CONFIG_PL_ETHER_MONITOR_STACKSIZE

// Monitor Thread Periodic
#ifdef CONFIG_PL_ETHER_MONITOR_PERIODIC_MS
# define ETHER_MONITOR_PERIODIC_MS        CONFIG_PL_ETHER_MONITOR_PERIODIC_MS
#else  // CONFIG_PL_ETHER_MONITOR_PERIODIC_MS
# define ETHER_MONITOR_PERIODIC_MS        (2 * 1000)
#endif  // CONFIG_PL_ETHER_MONITOR_PERIODIC_MS

// devive
#ifdef CONFIG_PL_ETH0_HAVE_DEVICE
# define ETH0_DEVICE_ENABLE               true
#else  // CONFIG_PL_ETH0_HAVE_DEVICE
# define ETH0_DEVICE_ENABLE               false
#endif  // CONFIG_PL_ETH0_HAVE_DEVICE

#ifdef CONFIG_PL_ETH0_DEVNAME
# define ETH0_DEVICE_NAME                 CONFIG_PL_ETH0_DEVNAME
#else  // CONFIG_PL_ETH0_DEVNAME
# define ETH0_DEVICE_NAME                 "\0"
#endif  // CONFIG_PL_ETH0_DEVNAME

#ifdef CONFIG_PL_ETH0_SPI_PORT
# define ETH0_SPI_PORT                    CONFIG_PL_ETH0_SPI_PORT
#else  // CONFIG_PL_ETH0_SPI_PORT
# define ETH0_SPI_PORT                    NETWORK_PORT_INVALID
#endif  // CONFIG_PL_ETH0_SPI_PORT

// ETH_RST
#ifdef CONFIG_PL_ETH0_RESET_PORT
# define ETH0_RESET_PORT                  CONFIG_PL_ETH0_RESET_PORT
#else  // CONFIG_PL_ETH0_RESET_PORT
# define ETH0_RESET_PORT                  NETWORK_PORT_INVALID
#endif  // CONFIG_PL_ETH0_RESET_PORT

#ifdef CONFIG_PL_ETH0_RESET_IS_IOEXP
# define ETH0_RESET_IS_IOEXP              true
#else  // CONFIG_PL_ETH0_RESET_IS_IOEXP
# define ETH0_RESET_IS_IOEXP              false
#endif  // CONFIG_PL_ETH0_RESET_IS_IOEXP

#ifdef CONFIG_PL_ETH0_RESET_ACTIVE_HIGH
# define ETH0_RESET_ACTIVE_HIGH           true
#else  // CONFIG_PL_ETH0_RESET_ACTIVE_HIGH
# define ETH0_RESET_ACTIVE_HIGH           false
#endif  // CONFIG_PL_ETH0_RESET_ACTIVE_HIGH

// ETH_IRQ
#ifdef CONFIG_PL_ETH0_IRQ_PORT
# define ETH0_IQR_PORT                    CONFIG_PL_ETH0_IRQ_PORT
#else  // CONFIG_PL_ETH0_IRQ_PORT
# define ETH0_IQR_PORT                    NETWORK_PORT_INVALID
#endif  // CONFIG_PL_ETH0_IRQ_PORT

#ifdef CONFIG_PL_ETH0_IRQ_IS_IOEXP
# define ETH0_IQR_IS_IOEXP                true
#else  // CONFIG_PL_ETH0_IRQ_IS_IOEXP
# define ETH0_IQR_IS_IOEXP                false
#endif  // CONFIG_PL_ETH0_IRQ_IS_IOEXP

#ifdef CONFIG_PL_ETH0_IRQ_ACTIVE_HIGH
# define ETH0_IQR_ACTIVE_HIGH             true
#else  // CONFIG_PL_ETH0_IRQ_ACTIVE_HIGH
# define ETH0_IQR_ACTIVE_HIGH             false
#endif  // CONFIG_PL_ETH0_IRQ_ACTIVE_HIGH

// ETH_PWR_EN
#ifdef CONFIG_PL_ETH0_POWER_PORT
# define ETH0_POWER_PORT                  CONFIG_PL_ETH0_POWER_PORT
#else  // CONFIG_PL_ETH0_POWER_PORT
# define ETH0_POWER_PORT                  NETWORK_PORT_INVALID
#endif  // CONFIG_PL_ETH0_POWER_PORT

#ifdef CONFIG_PL_ETH0_POWER_IS_IOEXP
# define ETH0_POWER_IS_IOEXP              true
#else  // CONFIG_PL_ETH0_POWER_IS_IOEXP
# define ETH0_POWER_IS_IOEXP              false
#endif  // CONFIG_PL_ETH0_POWER_IS_IOEXP

#ifdef CONFIG_PL_ETH0_POWER_ACTIVE_HIGH
# define ETH0_POWER_ACTIVE_HIGH           true
#else  // CONFIG_PL_ETH0_POWER_ACTIVE_HIGH
# define ETH0_POWER_ACTIVE_HIGH           false
#endif  // CONFIG_PL_ETH0_POWER_ACTIVE_HIGH

// Typdefs ---------------------------------------------------------------------


// Functions -------------------------------------------------------------------
PlErrCode PlEtherInitialize(struct network_info *net_info,
                            uint32_t thread_priority);
PlErrCode PlEtherFinalize(struct network_info *net_info);
PlErrCode PlEtherSetConfig(struct network_info *net_info,
                           const PlNetworkConfig *config);
PlErrCode PlEtherGetConfig(struct network_info *net_info,
                           PlNetworkConfig *config);
PlErrCode PlEtherGetStatus(struct network_info *net_info,
                           PlNetworkStatus *status);
PlErrCode PlEtherRegisterEventHandler(struct network_info *net_info);
PlErrCode PlEtherUnregisterEventHandler(struct network_info *net_info);
PlErrCode PlEtherStart(struct network_info *net_info);
PlErrCode PlEtherStop(struct network_info *net_info);

#endif  // PL_ETHER_H_
