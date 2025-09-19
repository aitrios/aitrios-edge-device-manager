# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

pl_sources += files([
  'pl_network_migration_os_impl.c',
  'pl_network_util_os_impl.c',
])

if config_h.get('CONFIG_PL_NETWORK_HAVE_ETHER', false)
  pl_sources += files(['pl_ether_os_impl.c',])
endif

if config_h.get('CONFIG_PL_NETWORK_HAVE_WIFI', false)
  if config_h.get('CONFIG_PL_NETWORK_WIFI_STATION', false)
    pl_sources += files(['pl_wifi_sta_os_impl.c',])
  endif
endif
