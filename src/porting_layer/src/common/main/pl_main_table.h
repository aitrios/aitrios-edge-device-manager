/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef PL_MAIN_TABLE_H__
#define PL_MAIN_TABLE_H__

#if defined(__NuttX__)
#include <nuttx/config.h>
#endif

#include <stdbool.h>

// typedef --------------------------------------------------------------------
// An enumeration type representing the categories of storage devices.
typedef enum PlMainDeviceType {
  kPlMainDeviceEmmc,   // It becomes the target of operations for APIs starting
                       // with PlMainEmmc.
  kPlMainDeviceFlash,  // It becomes the target of operations for APIs starting
                       // with PlMainFlash.
  kPlMainDeviceMax,
} PlMainDeviceType;

// An enumeration type representing the file system types of storage devices.
typedef enum PlMainFileSystemType {
  kPlMainFileSystemFat32,     // It can be used with eMMC.
  kPlMainFileSystemLittleFs,  // It can be used for Flash.
  kPlMainFileSystemOther,     // It can be used for Flash.
  kPlMainFileSystemMax,
} PlMainFileSystemType;

// This is a structure that defines information about memory devices.
typedef struct PlMainDeviceInformation {
  PlMainDeviceType device_type;  // Type of storage device.
  PlMainFileSystemType fs_type;  // Type of file system.
  const char* source;            // Device path. NULL cannot be specified.
  const char* target;  // Mount point. If NULL, it will not be processed by
                       // mount/unmount APIs.
  bool format;  // If true, formatting will be attempted by the format API.
                // If false, formatting will not be performed by the format API.
} PlMainDeviceInformation;

#endif  // PL_MAIN_TABLE_H__
