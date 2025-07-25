/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __APPS_INCLUDE_NETUTILS_NTPCLIENT_H
#define __APPS_INCLUDE_NETUTILS_NTPCLIENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#if !defined(FAR)
#define FAR
#endif
#ifdef __NuttX__
#include <nuttx/config.h>
#endif  // __NuttX__

#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

#ifndef CONFIG_NETUTILS_NTPCLIENT_SERVERIP
#define CONFIG_NETUTILS_NTPCLIENT_SERVERIP 0x0a000001
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_PORTNO
#define CONFIG_NETUTILS_NTPCLIENT_PORTNO 123
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_STACKSIZE
#define CONFIG_NETUTILS_NTPCLIENT_STACKSIZE 2048
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO
#define CONFIG_NETUTILS_NTPCLIENT_SERVERPRIO 100
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC
#define CONFIG_NETUTILS_NTPCLIENT_POLLDELAYSEC 60
#endif

#ifndef CONFIG_NETUTILS_NTPCLIENT_SIGWAKEUP
#define CONFIG_NETUTILS_NTPCLIENT_SIGWAKEUP 18
#endif

#define SANITY_OVER_CNT_MAX 65535U

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* This enumeration describes the state of the NTP daemon */

enum ntpc_daemon_e {
  NTP_NOT_RUNNING = 0,
  NTP_STARTED,
  NTP_RUNNING,
  NTP_STOP_REQUESTED,
  NTP_STOPPED
};

struct ntp_skip_and_limit_s {
  bool is_skip_and_limit_on;

  /* The unit of this value is milliseconds.
   * If the absolute value of delta of sample equals or less than this value,
   * the sample is used as an NTP sampling, where delta is defined in RFC 5905;
   * delta implies round-trip delay.
   */
  int32_t limit_packet_time;

  /* The unit of this value is milliseconds.
   * If the absolute value of theta of a sample is greater than this value, the
   * theta of the sample is changed to ``sgn(theta) * (this value)'', where
   * theta is defined in RFC 5905; theta implies offset. The range is [0, 1000].
   */
  int32_t limit_rtc_correction_value;

  /* The unit of this value is milliseconds.
   * If the absolute value of theta of a sample is greater than this value, the
   * sample is regarded as a singularity, where theta is defined in RFC 5905;
   * theta implies offset.
   */
  int32_t sanity_limit;
};

struct ntp_slew_param_s {
  bool is_slew_param_on;

  /* The unit of this value is milliseconds.
   * This value is a default interval value which NTP client sends time
   * synchronization messages for.
   */
  int32_t stable_rtc_correction_value_base;

  /* This value is a threshold of the number of counts to extend the interval
   * when it happens continuously that absolute value of theta equals or less
   * than an expected time value, in samplings when NTP client sends time
   * synchronization messages in the interval.
   */
  int stable_sync_number;
};

struct ntp_sync_params_s {
  int sync_interval_base;
  struct ntp_skip_and_limit_s skip_and_limit_params;
  struct ntp_slew_param_s slew_params;
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: ntpc_dualstack_family()
 *
 * Description:
 *   Set the protocol family used (AF_INET, AF_INET6 or AF_UNSPEC)
 *
 ****************************************************************************/

void ntpc_dualstack_family(int family);

/****************************************************************************
 * Name: ntpc_start_with_list
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start_with_list(FAR const char *ntp_server_list);

/****************************************************************************
 * Name: ntpc_start_with_params
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start_with_params(FAR const char *ntp_server_list,
                           FAR const struct ntp_sync_params_s *params);

/****************************************************************************
 * Name: ntpc_start
 *
 * Description:
 *   Start the NTP daemon
 *
 * Returned Value:
 *   On success, the non-negative task ID of the NTPC daemon is returned;
 *   On failure, a negated errno value is returned.
 *
 ****************************************************************************/

int ntpc_start(void);

/****************************************************************************
 * Name: ntpc_stop
 *
 * Description:
 *   Stop the NTP daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.  The current
 *   implementation only returns success.
 *
 ****************************************************************************/

int ntpc_stop(void);

/****************************************************************************
 * Name: ntpc_status
 *
 * Description:
 *   Get a status of the NTP daemon
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

struct ntpc_status_s {
  /* the latest samples */

  unsigned int nsamples;
  struct {
    int64_t offset;
    int64_t delay;
    FAR const struct sockaddr *srv_addr;
    struct sockaddr_storage _srv_addr_store;
  } samples[CONFIG_NETUTILS_NTPCLIENT_NUM_SAMPLES];
  pid_t pid;
  enum ntpc_daemon_e ntpc_daemon_state;
  unsigned int sanity_over_cnt;
  int num_of_errors;
};

int ntpc_status(struct ntpc_status_s *statusp);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __APPS_INCLUDE_NETUTILS_NTPCLIENT_H */
