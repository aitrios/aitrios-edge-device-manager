/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

#ifndef _COMPATIBILITY_H_

#ifndef up_puts
#define up_puts(x) puts(x)
#endif
#define get_errno() errno

#endif /* _COMPATIBILITY_H_ */
