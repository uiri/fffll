/* MyList - Linked List Implementation
   Copyright (C) 2012,2013 Uiri Noyb

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

#ifndef MYLISTSTRUCT
#define MYLISTSTRUCT

typedef struct List List;

struct List {
  List *next;
  void *data;
};

typedef struct ListAllocator ListAllocator;

struct ListAllocator {
  List** new;
  DynArray* free;
  int next;
  int pools;
  int maxpools;
};

#endif

extern const List EmptyList;

List* addListToList(List *list, List *oldlist);
List addToListAfterData(List *list, void *newdata, void *afterdata);
List addToListAfterDataLast(List *list, void *newdata, void *afterdata);
List addToListAtPosition(List *list, int position, void *data);
List addToListBeforeData(List *list, void *newdata, void *beforedata);
List addToListBeforeDataLast(List *list, void *newdata, void *beforedata);
List addToListBeginning(List *list, void *data);
List addToListEnd(List *list, void *data);
List* allocList();
List changeInListDataAtPosition(List *list, int position, void *data);
List *cloneList(List *list);
void *dataInListAtPosition(List *list, int position);
void *dataInListBeginning(List *list);
void *dataInListEnd(List *list);
List *deleteFromListBeginning(List *list);
List deleteFromListData(List *list, void *data);
List deleteFromListEnd(List *list);
List deleteFromListPosition(List *list, int position);
List deleteFromListLastData(List *list, void *data);
int freeList(List *list);
int freeListAlloc();
int freeListNode(List *list);
int lastPositionInListOfData(List *list, void *data);
int lengthOfList(List *list);
List *newList();
int positionInListOfData(List *list, void *data);
List sortList(List *list);
