/*
 * SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This header file defines structures and functions related to the HW info
// data management in the parameter storage manager.

#ifndef ESF_SYSTEM_MANAGER_ACCESSOR_MIGRATION_DATA_H_
#define ESF_SYSTEM_MANAGER_ACCESSOR_MIGRATION_DATA_H_

#include "parameter_storage_manager_common.h"
#include "system_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

EsfSystemManagerResult EsfSystemManagerMigrationData(
    PlSystemManagerMigrationDataId id, void *dst, size_t dst_size);

// """Migrates RootAuth data from old format to Parameter Storage Manager.
// This function retrieves RootAuth data from the migration source,
// calculates SHA-256 hash of the Root CA certificate, and saves both
// the certificate and its hash to the Parameter Storage Manager.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultInternalError: if the migration operation fails.
// """
EsfSystemManagerResult EsfSystemManagerMigrateRootAuth(void);

// """Migrates DeviceManifest data from old format to Parameter Storage Manager.
// This function retrieves DeviceManifest data from the migration source
// and saves it to the Parameter Storage Manager for use in the new system.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultInternalError: if the migration operation fails.
// """
EsfSystemManagerResult EsfSystemManagerMigrateDeviceManifest(void);

// """Migrates HwInfo data from old format to Parameter Storage Manager.
// This function retrieves hardware information data from the migration source
// and saves it to the Parameter Storage Manager for use in the new system.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultInternalError: if the migration operation fails.
// """
EsfSystemManagerResult EsfSystemManagerMigrateHwInfo(void);

// """Migrates EVP setup info data from old format to Parameter Storage Manager.
// This function retrieves EVP (Edge Video Processing) setup information
// from the migration source, parses the address and port configuration,
// and saves the extracted EVP configuration to the Parameter Storage Manager.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultInternalError: if the migration operation fails.
// """
EsfSystemManagerResult EsfSystemManagerMigrateEvpSetupInfo(void);

// """Erases migration data for the specified data ID.
// This function removes the old migration data file after successful migration.
// Returns:
//   kEsfSystemManagerResultOk: if the operation is successful.
//   kEsfSystemManagerResultInternalError: if the erase operation fails.
// """
EsfSystemManagerResult EsfSystemManagerEraseMigrationData(PlSystemManagerMigrationDataId id);

#ifdef __cplusplus
}
#endif

#endif  // ESF_SYSTEM_MANAGER_ACCESSOR_MIGRATION_DATA_H_
