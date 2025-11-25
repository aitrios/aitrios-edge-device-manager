# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

config_h = configuration_data()

subdir(get_option('target'))

# enable config for test if test option is on
if get_option('test_build')
  message('test_build option is enabled')
  subdir('test_' + get_option('target'))
else
  message('test_build option is disabled')
endif


common_arguments = [
	'-include', meson.current_build_dir() / 'config.h'
]
add_project_arguments(common_arguments, language : 'c')

configure_file(output : 'config.h',
	configuration : config_h,
)
