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

#include <stddef.h>

#ifndef MYDYNAMICARRAY
#define MYDYNAMICARRAY

typedef struct DynArray DynArray;

struct DynArray {
  int length;
  int last;
  size_t elementsize;
  void **array;
};

#endif

int appendToArray(DynArray* da, void* data);
int freeArray(DynArray* da);
void *getElementInArray(DynArray* da, int index);
DynArray *newArray(int len, size_t size);
void *popFromArray(DynArray* da);
int setElementInArray(DynArray *da, int index, void* data);
