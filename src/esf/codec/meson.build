# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

codec_sources = files([])
codec_includes_public = []
codec_includes_internal = []

subdir('base64')
subdir('jpeg')
subdir('json')

# Some consumers of the codec headers expect to navigate from this point, for
# example by calling #include "json/include/json.h". To accomodate that add this
# point as an include directory
codec_includes_public += include_directories('.')
