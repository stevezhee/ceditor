//
//  DynamicArray.h
//  ceditor
//
//  Created by Brett Letner on 5/27/20.
//  Copyright (c) 2021 Brett Letner. All rights reserved.
//

#ifndef DynamicArray_h
#define DynamicArray_h

#include "Util.h"

void arrayGrow(dynamicArray_t *arr, int maxElems);
void arrayReinit(dynamicArray_t *arr);
void arrayInit(dynamicArray_t *arr, int elemSize);
void *arrayPushUninit(dynamicArray_t *arr);
void arrayPush(dynamicArray_t *arr, void *elem);
void *arrayPop(dynamicArray_t *arr);
void *arrayElemAt(dynamicArray_t *arr, int i);
void *arrayFocus(dynamicArray_t *arr);
void arraySetFocus(dynamicArray_t *arr, int i);
void *arrayBoundary(dynamicArray_t *arr);
void arrayInsert(dynamicArray_t *arr, int offset, void *s, int len);
int arrayDelete(dynamicArray_t *arr, int offset, int len);
void *arrayTop(dynamicArray_t *arr);
void arrayFree(dynamicArray_t *arr);
bool arrayAtTop(dynamicArray_t *arr);
int arrayMaxSize(dynamicArray_t *arr);
char *cstringOf(string_t *s);

#endif /* DynamicArray_h */
