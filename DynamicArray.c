//
//  DynamicArray.c
//  editordebugger
//
//  Created by Brett Letner on 5/27/20.
//  Copyright © 2020 Brett Letner. All rights reserved.
//

#include "DynamicArray.h"

// BAL: we seem to call this function way too much ... probably undo causing
// problems(?)
void arrayGrow(dynamicArray_t *arr, int maxElems) {
  assert(arr);
  assert(maxElems >= 1);
  assert(arr->elemSize > 0);

  printf("arrayGrow on %p\n", arr);

  if (maxElems <= arr->maxElems)
    return;

  int n = maxElems; //BAL: max(maxElems, arr->maxElems * 2);
  // printf("growing array %d elements (%d) was (%d)\n", n, maxElems, arr->maxElems);

  int oldSize = arr->elemSize * arr->maxElems;

  int sz = arr->elemSize * n;

  printf("attempting to reallocate %p %d bytes (elem size = %d)\n", arr->start, sz, arr->elemSize);

  void *p = realloc(arr->start, sz);
  printf("realloc worked...\n");
  arr->start = dieIfNull(p);
  assert(arr->start == p);
  printf("attempting to memset %p at %p (%d bytes)\n", arr->start, arr->start + oldSize, sz - oldSize);
  memset(arr->start + oldSize, 0, sz - oldSize); // BAL: zeroing memory - remove
  arr->maxElems = n;
  printf("memset worked...\n");
  printf("allocate complete\n");
}

void arrayReinit(dynamicArray_t *arr) {
  assert(arr);
  arr->offset = 0;
  arr->numElems = 0;
}

void arrayInit(dynamicArray_t *arr, int elemSize) {
  memset(arr, 0, sizeof(dynamicArray_t));
  arr->elemSize = elemSize;
}

#define elemAt(arr, i) ((arr)->start + (i) + (arr)->elemSize)

void *arrayElemAt(dynamicArray_t *arr, int i) {
  assert(arr);
  assert(arr->start);
  assert(i >= 0);
  assert(i < arr->numElems);
  return elemAt(arr, i);
}

void *arrayFocus(dynamicArray_t *arr) { return arrayElemAt(arr, arr->offset); }

void *arrayTop(dynamicArray_t *arr) {
  assert(arr);
  assert(arr->start);
  return elemAt(arr, arr->numElems);
}

bool arrayAtTop(dynamicArray_t *arr) {
  assert(arr->offset >= 0);
  assert(arr->offset <= arr->numElems);
  return arr->offset == arr->numElems;
}

int arrayDelete(dynamicArray_t *arr, int offset, int len0) {
  assert(arr);
  int len = min(len0, arr->numElems - offset);

  void *p = arrayElemAt(arr, offset);
  void *q = p + len * arr->elemSize;

  assert(arrayTop(arr) >= q);
  memmove(p, q, arrayTop(arr) - q);
  arr->numElems -= len;
  return len;
}

void arrayInsert(dynamicArray_t *arr, int offset, void *s, int len) {
  assert(arr);
  int n = arr->numElems + len;
  if (n > arr->maxElems)
    arrayGrow(arr, n);

  int sz = len * arr->elemSize;
  void *top = arrayTop(arr); // needs to be before numElems is changed

  arr->numElems = n; // needs to be before call to arrayElemAt
  void *p = arrayElemAt(arr, offset);

  memmove(p + sz, p, top - p);
  memcpy(p, s, sz);
}

void *arrayPushUninit(dynamicArray_t *arr) {
  assert(arr);
  assert(arr->numElems >= 0);
  assert(arr->numElems <= arr->maxElems);
  if (arr->numElems == arr->maxElems) {
    arrayGrow(arr, 1 + arr->maxElems);
  }
  assert(arr->start);
  void *p = arrayTop(arr);
  arr->numElems++;
  return p;
}

void arrayPush(dynamicArray_t *arr, void *elem) {
  assert(arr);
  assert(elem);
  memcpy(arrayPushUninit(arr), elem, sizeof(arr->elemSize));
}

void *arrayPop(dynamicArray_t *arr) {
  assert(arr);
  assert(arr->start);
  assert(arr->numElems > 0);
  arr->numElems--;
  return arrayTop(arr);
}

void arraySetFocus(dynamicArray_t *arr, int i) {
  assert(arr);
  assert(i >= 0);
  assert(i < arr->numElems);
  arr->offset = i;
}

void arrayFree(dynamicArray_t *arr) {
  // BAL: printf("freeing array...\n");
  free(arr->start);
}
