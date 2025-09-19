# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

pl_sources += files([
  'pl_network_migration.c',
  'pl_network.c',
  'pl_network_util.c',
])

if config_h.get('CONFIG_PL_NETWORK_HAVE_ETHER', false)
  pl_sources += files(['pl_ether.c',])
endif

if config_h.get('CONFIG_PL_NETWORK_HAVE_WIFI', false)
  pl_sources += files(['pl_wifi.c',])
  if config_h.get('CONFIG_PL_NETWORK_WIFI_STATION', false)
    pl_sources += files(['pl_wifi_sta.c',])
  endif
  if config_h.get('CONFIG_PL_NETWORK_WIFI_AP', false)
    pl_sources += files(['pl_wifi_ap.c',])
  endif
endif
