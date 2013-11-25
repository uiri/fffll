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
int reallocArray(DynArray* da);
int setElementInArray(DynArray *da, int index, void* data);
