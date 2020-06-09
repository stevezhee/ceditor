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
  assert(arr);
  assert(maxElems >= 1);
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

void *arrayElemAt(dynamicArray_t *arr, int i)
{
  assert(arr);
  assert(arr->start);
  // BAL: assert(i >= 0);
  // BAL: assert(i < arr->numElems);

  return arr->start + i * arr->elemSize;
}

int arrayFocusOffset(dynamicArray_t *arr)
{
  return arr->offset;
}

void *arrayFocus(dynamicArray_t *arr)
{
  return arrayElemAt(arr, arrayFocusOffset(arr));
}

void *arrayTop(dynamicArray_t *arr)
{
  return arr->start + arr->numElems * arr->elemSize;
}

void arrayDelete(dynamicArray_t *arr, int offset, int len)
{
  assert(arr);
  void *p = arrayElemAt(arr, offset);
  void *q = p + len*arr->elemSize;
  memmove(p, q, arrayTop(arr) - q);
  arr->numElems -= len;
}

void arrayInsert(dynamicArray_t *arr, int offset, void *s, int len)
{
  assert(arr);
  int n = arr->numElems + len;
  if (n > arr->maxElems) arrayGrow(arr, n);

  int sz = len*arr->elemSize;

  void *p = arrayElemAt(arr, offset);

  memmove(p + sz, p, arrayTop(arr) - p);
  memcpy(p, s, sz);

  arr->numElems = n;
}

void *arrayPushUninit(dynamicArray_t *arr)
{
  assert(arr);
  if (arr->numElems == arr->maxElems)
    {
      arrayGrow(arr, 1 + arr->maxElems * 2);
    }
  assert(arr->start);
  void *p = arrayTop(arr);
  arr->numElems++;
  return p;
}

void arrayPush(dynamicArray_t *arr, void *elem)
{
  assert(arr);
  assert(elem);
  memcpy(arrayPushUninit(arr), elem, sizeof(arr->elemSize));
}

void *arrayPop(dynamicArray_t *arr)
{
  assert(arr);
  assert(arr->start);
  assert(arr->numElems > 0);
  arr->numElems--;
  return arrayTop(arr);
}

void *arrayPeek(dynamicArray_t *arr)
{
  assert(arr);
  return arrayElemAt(arr, arr->numElems - 1);
}

void arraySetFocus(dynamicArray_t *arr, int i)
{
  assert(arr);
  assert(i >= 0);
  assert(i < arr->numElems);
  arr->offset = i;
}
