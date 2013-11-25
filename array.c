#include "array.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

int appendToArray(DynArray* da, void* data) {
  if (da->last == da->length) {
    reallocArray(da);
  }
  da->array[da->last*da->elementsize] = data;
  da->last++;
  return 0;
}

int freeArray(DynArray* da) {
  free(da->array);
  free(da);
  return 0;
}

void *getElementInArray(DynArray* da, int index) {
  if (index >= da->last)
    return NULL;
  return da->array[index*da->elementsize];
}

DynArray *newArray(int len, size_t size) {
  DynArray *da;
  da = malloc(sizeof(DynArray));
  da->length = len;
  da->last = 0;
  da->elementsize = size;
  da->array = calloc(len, size);
  return da;
}

int reallocArray(DynArray* da) {
  da->length *= 2;
  printf("%p\n", (void*)da->array);
  da->array = realloc(da->array, da->length*da->elementsize);
  printf("%p\n", (void*)da->array);
  return 0;
}

int setElementInArray(DynArray *da, int index, void* data) {
  if (index < 0 || index >= da->last) {
    return -1;
  }
  da->array[index*da->elementsize] = data;
  return 0;
}
