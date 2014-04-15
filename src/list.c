/* MyList - Linked List Implementation
   Copyright (C) 2012-2014 Uiri Noyb

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
#include "list.h"
#include "array.h"
#include <stdlib.h>
#include <stdio.h>

const List EmptyList = {NULL, NULL};

ListAllocator* ListManager = NULL;

List* addListToList(List *list, List *oldlist) {
  List *headptr;
  headptr = list;
  while (oldlist != NULL) {
    addToListEnd(list, oldlist->data);
    oldlist = oldlist->next;
  }
  return headptr;
}

List addToListAfterData(List *list, void *newdata, void *afterdata) {
  List *headptr, *oldptr;
  headptr = list;
  if (list->data != afterdata)
    while (list->data != afterdata) {
      if (list->next == NULL)
	return *headptr;
      list = list->next;
    }
  oldptr = list->next;
  list->next = allocList();
  list->next->next = oldptr;
  list->next->data = newdata;
  return *headptr;
}

List addToListAfterDataLast(List *list, void *newdata, void *afterdata) {
  List *headptr, *lastptr;
  if (list == NULL)
    return EmptyList;
  headptr = list;
  lastptr = NULL;
  while (list != NULL) {
    if (list->data == afterdata)
      lastptr = list;
    list = list->next;
  }
  if (lastptr == NULL)
    return *headptr;
  list = lastptr;
  lastptr = list->next;
  list->next = allocList();
  list->next->next = lastptr;
  list->next->data = newdata;
  return *headptr;
}

List addToListBeforeData(List *list, void *newdata, void *beforedata) {
  List *headptr, *oldptr;
  if (list == NULL)
    return EmptyList;
  headptr = list;
  while (list != NULL) {
    if (list->data == beforedata)
      break;
    list = list->next;
  }
  if (list == NULL)
    return *headptr;
  oldptr = list->next;
  list->next = allocList();
  list->next->data = beforedata;
  list->next->next = oldptr;
  list->data = newdata;
  return *headptr;
}

List addToListBeforeDataLast(List *list, void *newdata, void *beforedata) {
  List *headptr, *lastptr;
  if (list == NULL)
    return EmptyList;
  headptr = list;
  lastptr = NULL;
  while (list != NULL) {
    if (list->data == beforedata)
      lastptr = list;
    list = list->next;
  }
  if (lastptr == NULL)
    return *headptr;
  list = lastptr;
  lastptr = list->next;
  list->next = allocList();
  list->next->next = lastptr;
  list->next->data = beforedata;
  list->data = newdata;
  return *headptr;
}

List addToListAtPosition(List *list, int position, void *data) {
  List *headptr, *oldptr;
  int i;
  headptr=list;
  for(i=0;i<position;i++)
    list = list->next;
  oldptr = list->next;
  list->next = allocList();
  list->next->next = oldptr;
  list->next->data = data;
  return *headptr;
}

List addToListBeginning(List *list, void *data) {
  List *headptr, *oldptr;
  headptr = list;
  oldptr = list->next;
  list->next = allocList();
  list->next->next = oldptr;
  list->next->data = list->data;
  list->data = data;
  return *headptr;
}

List addToListEnd(List *list, void *data) {
  List *headptr;
  headptr = list;
  if (list->data == NULL) {
    list->data = data;
    return *headptr;
  }
  while(list->next != NULL)
    list = list->next;
  list->next = allocList();
  list->next->data = data;
  list->next->next = NULL;
  return *headptr;
}

List* allocList() {
  if (ListManager == NULL) {
    ListManager = malloc(sizeof(ListAllocator));
    ListManager->pools = 0;
    ListManager->maxpools = 4;
    /* alloc ListManager->new and ListManager->new[0] */
    ListManager->new = malloc((ListManager->maxpools)*sizeof(List*));
    ListManager->new[ListManager->pools] = calloc(16, sizeof(List));
    ListManager->free = newArray(8, sizeof(List*));
    ListManager->next = 0;
    return ListManager->new[ListManager->pools]+ListManager->next++;
  }
  if (ListManager->free->last != 0) {
    return popFromArray(ListManager->free);
  }
  if (ListManager->next == 16) {
    ListManager->pools++;
    if (ListManager->pools == ListManager->maxpools) {
      ListManager->maxpools += 4;
      ListManager->new = realloc(ListManager->new, (ListManager->maxpools)*sizeof(List*));
    }
    ListManager->new[ListManager->pools] = calloc(16, sizeof(List));
    ListManager->next = 0;
  }
  return ListManager->new[ListManager->pools]+ListManager->next++;
}

List changeInListDataAtPosition(List *list, int position, void *data) {
  List *headptr;
  int i;
  headptr = list;
  for(i=0;i<position;i++)
    list = list->next;
  list->data = data;
  return *headptr;
}

List *cloneList(List *list) {
  List *clone, *headptr;
  if (list == NULL)
    return NULL;
  clone = allocList();
  clone->data = list->data;
  clone->next = NULL;
  headptr = clone;
  while (list->next != NULL) {
    list = list->next;
    clone->next = allocList();
    clone->next->data = list->data;
    clone->next->next = NULL;
    clone = clone->next;
  }
  return headptr;
}

void *dataInListAtPosition(List *list, int position) {
  int i;
  for (i=0;i<position;i++)
    list = list->next;
  return list->data;
}

void *dataInListBeginning(List *list) {
  return list->data;
}

void *dataInListEnd(List *list) {
  while (list->next != NULL)
    list = list->next;
  return list->data;
}

List deleteFromListData(List *list, void *data) {
  List *headptr, *rmptr;
  headptr = list;
  if (list->data == data) {
    rmptr = list->next;
    list->data = list->next->data;
    list->next = list->next->next;
    freeListNode(rmptr);
    return *headptr;
  }
  while (list->next->data != data) {
    list = list->next;
    if (list->next == NULL)
      return *headptr;
  }
  rmptr = list->next;
  list->next = list->next->next;
  freeListNode(rmptr);
  return *headptr;
}

List deleteFromListEnd(List *list) {
  List *headptr, *lastptr;
  headptr = list;
  lastptr = NULL;
  while (list->next != NULL) {
    lastptr = list;
    list = list->next;
  }
  if (list == headptr)
    return *headptr;
  else {
    freeListNode(list);
    lastptr->next = NULL;
  }
  return *headptr;
}

List *deleteFromListBeginning(List *list) {
  List *headptr;
  headptr = list->next;
  freeListNode(list);
  return headptr;
}

List deleteFromListPosition(List *list, int position) {
  List *headptr, *rmptr;
  int i;
  headptr = list;
  if (position == 0) {
    list->data = NULL;
    return *headptr;
  }
  position--;
  for(i=0;i<position;i++)
    list = list->next;
  rmptr = list->next;
  list->next = list->next->next;
  freeListNode(rmptr);
  return *headptr;
}

List deleteFromListLastData(List *list, void *data) {
  List *headptr, *lastptr;
  if (list == NULL)
    return EmptyList;
  headptr = list;
  lastptr = NULL;
  while (list != NULL) {
    if (list->data == data)
      lastptr = list;
    list=list->next;
  }
  if (lastptr == NULL)
    return *headptr;
  list = lastptr;
  lastptr = list->next;
  if (lastptr == NULL)
    return *headptr;
  if (list == headptr) {
    list->data = lastptr->data;
    if (lastptr == NULL)
      list->data = NULL;
  }
  list->next = lastptr->next;
  freeListNode(lastptr);
  return *headptr;
}

int freeList(List *list) {
  /* List *headptr; */
  List *nextptr;
  /* headptr = list; */
  if (list == NULL) return 0;
  while (list != NULL) {
    nextptr = list->next;
    list->next = NULL;
    freeListNode(list);
    list = nextptr;
  }
  return 0;
}

int freeListAlloc() {
  int i;
  freeArray(ListManager->free);
  for (i=0;i<=ListManager->pools;i++) {
    free(ListManager->new[i]);
  }
  free(ListManager->new);
  free(ListManager);
  return 0;
}

int freeListNode(List *list) {
  list->data = NULL;
  if (ListManager->next && list == ListManager->new[ListManager->pools]+(ListManager->next-1)) {
    ListManager->next--;
    if (!ListManager->next) {
      ListManager->next = 16;
      free(ListManager->new[ListManager->pools]);
      ListManager->pools--;
    }
  } else {
    appendToArray(ListManager->free, list);
  }
  return 0;
}

int lastPositionInListOfData(List *list, void *data) {
  int i,j=-1;
  for(i=0;list!=NULL;list=list->next) {
    if (list->data == data)
      j=i;
    i++;
    if (list->next == NULL)
      break;
  }
  return j;
}

int lengthOfList(List *list) {
  int i;
  if (list == NULL) return 0;
  if (list->data == NULL && list->next == NULL) return 0;
  for(i=1;list->next!=NULL;list=list->next) {
    i++;
  }
  return i;
}

List *newList() {
  List *list;
  list = allocList();
  *list = EmptyList;
  return list;
}

int positionInListOfData(List *list, void *data) {
  int i;
  for(i=0;list!=NULL;list=list->next) {
    if (list->data == data)
      break;
    i++;
    if (list->next == NULL)
      i = -1;
  }
  return i;
}

List sortList(List *list) {
  List *headptr, *pivotptr, *next;
  void *pivot, *tmpdata;
  headptr = list;
  pivotptr = list;
  while (1) {
    pivot = list->data;
    next = list->next;
    while (next != NULL) {
      if (*(int *)pivot > *(int *)next->data) {
	tmpdata = next->data;
	next->data = pivot;
	list->data = tmpdata;
	list = next;
	next = list->next;
      } else
	next = next->next;
    }
    if (pivot == pivotptr->data)
      pivotptr = pivotptr->next;
    if (pivotptr == NULL)
      break;
    list = pivotptr;
  }
  return *headptr;
}
