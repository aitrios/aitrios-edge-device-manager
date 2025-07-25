/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

// Define the linked list API for ButtonManager code.

#ifndef ESF_BUTTON_MANAGER_SRC_BUTTON_MANAGER_LINKED_LIST_H_
#define ESF_BUTTON_MANAGER_SRC_BUTTON_MANAGER_LINKED_LIST_H_

#include <stdint.h>

#include "button_manager.h"

// A structure that defines the elements of a linked list.
typedef struct EsfButtonManagerCell {
  // Arbitrary data.
  void* data;

  // Address of the next element.
  struct EsfButtonManagerCell* next;
} EsfButtonManagerCell;

// A structure that defines a linked list.
typedef struct {
  // Address of the header cell.
  EsfButtonManagerCell* top;

  // Number of elements in linked list.
  int32_t size;
} EsfButtonManagerList;

// """Create a list.

// Create a list.

// Returns:
//     EsfButtonManagerList*: This is the list created by this function.

// """
EsfButtonManagerList* EsfButtonManagerCreateList(void);

// """Destroy the list.

// Destroy the list.

// Args:
//     list (EsfButtonManagerList**): Please specify the list created in
//       EsfButtonManagerCreateList().

// Note:
//     ***CAUTION***
//     This function must call after the free all cells and registered data.
//     This is because lists do not know the type of data themselves.

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns if Destroy cell fail.

// """
EsfButtonManagerStatus EsfButtonManagerDestroyList(EsfButtonManagerList** list);

// """Create a cell.

// Creates a cell containing the specified data.

// Args:
//     data (void*): arbitrary data.

// Returns:
//     EsfButtonManagerCell*: Returns the address of cell. If allocate failed,
//       return NULL.

// """
EsfButtonManagerCell* EsfButtonManagerCreateCell(void* data);

// """Destroy the cell.

// Destroy the cell.

// Args:
//     cell (EsfButtonManagerCell**): Please specify the cell created in
//       EsfButtonManagerCreateCell().

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if cell is NULL.

// """
EsfButtonManagerStatus EsfButtonManagerDestroyCell(EsfButtonManagerCell** cell);

// """push the cell.

// Pushes the cell to a list.

// Args:
//     cell (EsfButtonManagerCell*): Please specify the cell created in
//       EsfButtonManagerCreateCell().
//     list (EsfButtonManagerList*): Please specify the list created in
//       EsfButtonManagerCreateList().

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if cell or list is NULL.
//     kEsfButtonManagerStatusInternalError: Returns if push failed.

// """
EsfButtonManagerStatus EsfButtonManagerPushCell(EsfButtonManagerCell* cell,
                                                EsfButtonManagerList* list);

// """Remove the cell.

// Removes the specified element from the specified list.

// Args:
//     cell (EsfButtonManagerCell**): Please specify the cell created in
//       EsfButtonManagerCreateCell().
//     list (EsfButtonManagerList*): Please specify the list created in
//       EsfButtonManagerCreateList().

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if cell, list or list->top is
//       NULL or the specified element is not found in the list.

// """
EsfButtonManagerStatus EsfButtonManagerRemoveCell(EsfButtonManagerCell** cell,
                                                  EsfButtonManagerList* list);

// """Verify the cell.

// Verifies whether the specified element exists in the specified list.

// Args:
//     check_cell (const EsfButtonManagerCell*): Please specify the cell created
//       in EsfButtonManagerCreateCell().
//     list (const EsfButtonManagerList*): Please specify the list created in
//       EsfButtonManagerCreateList().

// Returns:
//     EsfButtonManagerStatus: The code returns one of the values
//       EsfButtonManagerStatus depending on the execution result.
//       Returns the value kEsfButtonManagerStatusOk if the process is
//       successful.

// Raises:
//     kEsfButtonManagerStatusParamError: Returns if check_cell or list is NULL
//       or the specified element is not found in the list.

// """
EsfButtonManagerStatus EsfButtonManagerVerifyCell(
    const EsfButtonManagerCell* check_cell, const EsfButtonManagerList* list);

// """Get the first cell.

// Gets the first element from the specified list.

// Args:
//     list (const EsfButtonManagerList*): Please specify the list created in
//       EsfButtonManagerCreateList().

// Returns:
//     EsfButtonManagerCell*: Returns the address of first element from the
//       specified list.

// """
EsfButtonManagerCell* EsfButtonManagerGetFrontCell(
    const EsfButtonManagerList* list);

// """Get the next cell.

// Gets the next element of the specified element.

// Args:
//     current_cell (const EsfButtonManagerCell*): Please specify the cell
//       created in EsfButtonManagerCreateCell().

// Returns:
//     EsfButtonManagerCell*: Returns the address of next element of the
//       specified element.

// """
EsfButtonManagerCell* EsfButtonManagerGetNextCell(
    const EsfButtonManagerCell* current_cell);

#endif  // ESF_BUTTON_MANAGER_SRC_BUTTON_MANAGER_LINKED_LIST_H_
