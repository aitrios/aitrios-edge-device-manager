/*
* SPDX-FileCopyrightText: 2024-2025 Sony Semiconductor Solutions Corporation
*
* SPDX-License-Identifier: Apache-2.0
*/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "button_manager_linked_list.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "button_manager.h"
#include "button_manager_internal.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

EsfButtonManagerList* EsfButtonManagerCreateList(void) {
  EsfButtonManagerList* list =
      (EsfButtonManagerList*)malloc(sizeof(EsfButtonManagerList));
  if (list == (EsfButtonManagerList*)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return (EsfButtonManagerList*)NULL;
  }

  list->top = (EsfButtonManagerCell*)NULL;
  list->size = 0;

  return list;
}

EsfButtonManagerStatus EsfButtonManagerDestroyList(
    EsfButtonManagerList** list) {
  if ((list == (EsfButtonManagerList**)NULL) ||
      (*list == (EsfButtonManagerList*)NULL)) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. List is null.");
    return kEsfButtonManagerStatusParamError;
  }

  free(*list);
  *list = (EsfButtonManagerList*)NULL;

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerCell* EsfButtonManagerCreateCell(void* data) {
  if (data == (void*)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Data is null.");
    return (EsfButtonManagerCell*)NULL;
  }

  EsfButtonManagerCell* cell =
      (EsfButtonManagerCell*)malloc(sizeof(EsfButtonManagerCell));
  if (cell == (EsfButtonManagerCell*)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Malloc error.");
    return (EsfButtonManagerCell*)NULL;
  }

  cell->data = data;
  cell->next = (EsfButtonManagerCell*)NULL;

  return cell;
}

EsfButtonManagerStatus EsfButtonManagerDestroyCell(
    EsfButtonManagerCell** cell) {
  if ((cell == (EsfButtonManagerCell**)NULL) ||
      (*cell == (EsfButtonManagerCell*)NULL)) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Cell is null.");
    return kEsfButtonManagerStatusParamError;
  }

  free(*cell);
  *cell = (EsfButtonManagerCell*)NULL;

  return kEsfButtonManagerStatusOk;
}

EsfButtonManagerStatus EsfButtonManagerPushCell(EsfButtonManagerCell* cell,
                                                EsfButtonManagerList* list) {
  if ((cell == (EsfButtonManagerCell*)NULL) ||
      (list == (EsfButtonManagerList*)NULL)) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Cell or list is null. cell=%p list=%p", cell, list);
    return kEsfButtonManagerStatusParamError;
  }

  // Push the cell to the list.
  EsfButtonManagerCell* search = list->top;
  // If top is NULL, the element is 0, so push it as is.
  if (search == (EsfButtonManagerCell*)NULL) {
    list->top = cell;
    ++list->size;
    return kEsfButtonManagerStatusOk;
  }
  // If search->next is NULL, it is judged as the end of the list, the cell is
  // set and the size is updated, and the process returns. If it is not found,
  // it cannot be added to the list and returns NULL as an error.
  for (int32_t i = 0; i < list->size; i++) {
    if (search->next == (EsfButtonManagerCell*)NULL) {
      search->next = cell;
      ++list->size;
      return kEsfButtonManagerStatusOk;
    }
    search = search->next;
  }

  return kEsfButtonManagerStatusInternalError;
}

EsfButtonManagerStatus EsfButtonManagerRemoveCell(EsfButtonManagerCell** cell,
                                                  EsfButtonManagerList* list) {
  if ((cell == (EsfButtonManagerCell**)NULL) ||
      (*cell == (EsfButtonManagerCell*)NULL) ||
      (list == (EsfButtonManagerList*)NULL)) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Cell or list is null. cell=%p list=%p", cell, list);
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell* search = list->top;
  if (search == (EsfButtonManagerCell*)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Top is null.");
    return kEsfButtonManagerStatusParamError;
  }

  // Delete the cell from the list.
  EsfButtonManagerCell* previous = (EsfButtonManagerCell*)NULL;
  for (int32_t i = 0;
       (i < list->size) && (search != (EsfButtonManagerCell*)NULL); i++) {
    if (search == *cell) {
      if (previous == (EsfButtonManagerCell*)NULL) {
        // Update list->top pointer.
        list->top = search->next;
      } else {
        previous->next = (*cell)->next;
      }
      --list->size;
      free(*cell);
      *cell = (EsfButtonManagerCell*)NULL;
      return kEsfButtonManagerStatusOk;
    }
    previous = search;
    search = search->next;
  }

  return kEsfButtonManagerStatusParamError;
}

EsfButtonManagerStatus EsfButtonManagerVerifyCell(
    const EsfButtonManagerCell* check_cell, const EsfButtonManagerList* list) {
  if ((check_cell == (EsfButtonManagerCell*)NULL) ||
      (list == (EsfButtonManagerList*)NULL)) {
    ESF_BUTTON_MANAGER_ERROR(
        "Invalid parameter. Cell or list is null. check_cell=%p list=%p",
        check_cell, list);
    return kEsfButtonManagerStatusParamError;
  }

  EsfButtonManagerCell* search = EsfButtonManagerGetFrontCell(list);
  for (int32_t i = 0; i < list->size; i++) {
    if (search == check_cell) {
      return kEsfButtonManagerStatusOk;
    }
    search = EsfButtonManagerGetNextCell(search);
  }
  return kEsfButtonManagerStatusParamError;
}

EsfButtonManagerCell* EsfButtonManagerGetFrontCell(
    const EsfButtonManagerList* list) {
  if (list == (EsfButtonManagerList*)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. List is null.");
    return (EsfButtonManagerCell*)NULL;
  }

  return list->top;
}

EsfButtonManagerCell* EsfButtonManagerGetNextCell(
    const EsfButtonManagerCell* current_cell) {
  if (current_cell == (EsfButtonManagerCell*)NULL) {
    ESF_BUTTON_MANAGER_ERROR("Invalid parameter. Cell is null.");
    return (EsfButtonManagerCell*)NULL;
  }

  return current_cell->next;
}
