//
//  DynamicArray.c
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "DynamicArray.h"

void arrayGrow(dynamicArray_t *arr, int maxElems)
{
  maxElems = max(1, maxElems);
  if (maxElems <= arr->maxElems) return;
  arr->maxElems = maxElems;
  arr->start = dieIfNull(realloc(arr->start, arr->elemSize * maxElems));
}

void arrayReinit(dynamicArray_t *arr)
{
  arr->offset = 0;
  arr->numElems = 0;
}

void arrayInit(dynamicArray_t *arr, int elemSize)
{
  memset(arr, 0, sizeof(dynamicArray_t));
  arr->elemSize = elemSize;
}

void *arrayPushUninit(dynamicArray_t *arr)
{
  if (arr->numElems == arr->maxElems)
    {
      arrayGrow(arr, arr->maxElems * 2);
    }
  assert(arr->start);
  void *p = arr->start + arr->numElems * arr->elemSize;
  arr->numElems++;
  return p;
}

void arrayPush(dynamicArray_t *arr, void *elem)
{
  memcpy(arrayPushUninit(arr), elem, sizeof(arr->elemSize));
}

void *arrayBoundary(dynamicArray_t *arr)
{
  return arr->start + arr->numElems * arr->elemSize;
}

void *arrayPop(dynamicArray_t *arr)
{
  assert(arr->start);
  if (arr->numElems == 0) return NULL;
  arr->numElems--;
  return (arr->start + arr->numElems * arr->elemSize);
}

void *arrayElemAt(dynamicArray_t *arr, int i)
{
  if (i < 0 || !arr->start || i >= arr->numElems)
    {
      return NULL;
    }

  return arr->start + i * arr->elemSize;
}

void *arrayPeek(dynamicArray_t *arr)
{
  return arrayElemAt(arr, arr->numElems - 1);
}

int arrayFocusOffset(dynamicArray_t *arr)
{
  return arr->offset;
}

void *arrayFocus(dynamicArray_t *arr)
{
  return arrayElemAt(arr, arrayFocusOffset(arr));
}

void arraySetFocus(dynamicArray_t *arr, int i)
{
  arr->offset = clamp(0, i, (int)arr->numElems - 1);
}
