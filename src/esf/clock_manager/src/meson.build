# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

if get_option('target') == 't4r'
esf_sources += files([
	'clock_manager_internal.c',
	'clock_manager_monitor.c',
	'clock_manager_notification.c',
	'clock_manager_setting.c',
	'clock_manager_utility.c',
	'clock_manager.c'
])
else
esf_sources += files([
	'clock_manager_utility.c',
	'clock_manager.c',
])
endif
