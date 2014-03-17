/* DynArray - Dynamic Array Implementation
   Copyright (C) 2013 W Pearson

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "array.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>


/* No need to expose this */

int reallocArray(DynArray* da) {
  da->length *= 2;
  da->array = realloc(da->array, da->length*da->elementsize);
  return 0;
}

/* Exposed functions */

int appendToArray(DynArray* da, void* data) {
  if (da->last == da->length) {
    reallocArray(da);
  }
  da->array[da->last] = data;
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
  return da->array[index];
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

void *popFromArray(DynArray* da) {
  void* data;
  data = da->array[--(da->last)];
  da->array[da->last] = NULL;
  return data;
}

int setElementInArray(DynArray *da, int index, void* data) {
  if (index < 0 || index >= da->last) {
    return -1;
  }
  da->array[index] = data;
  return 0;
}
