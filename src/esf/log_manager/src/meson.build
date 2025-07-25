# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

esf_sources += files([
	'log_manager_internal.c',
	'log_manager_internal.h',
	'log_manager_setting.c',
	'log_manager_setting.h',
	'log_manager.c'
])

# If LOG_MANAGER_EVP_ENABLE is disabled, use the stub of evp.
if not config_h.get('LOG_MANAGER_EVP_ENABLE')
	esf_sources += files([
		'stub/log_manager_stub.c',
	])

	esf_includes_internal += include_directories('stub/include')
endif

# If CONFIG_EXTERNAL_DLOG_DISABLE is disabled, use the list.
if not config_h.get('CONFIG_EXTERNAL_DLOG_DISABLE')
	esf_sources += files([
		'log_manager_list.c',
		'log_manager_list.h',
	])
endif

# If CONFIG_EXTERNAL_LOG_MANAGER_METRICS is abled, use the metrics.
if config_h.get('CONFIG_EXTERNAL_LOG_MANAGER_METRICS')
	esf_sources += files([
		'log_manager_metrics.c',
		'log_manager_metrics.h',
	])
endif

subdir('bytebuffer')
