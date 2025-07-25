# SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
#
# SPDX-License-Identifier: Apache-2.0

# The subdir directives collate source and include files into various lists. A
# number of circular dependencies within the structure of the project mandate
# that we complete that task before attempting compilation.

esf_includes_internal = []
esf_includes_internal += include_directories('include')

subdir('configs')
subdir('src')

if get_option('test_build')
    subdir('test')
endif
