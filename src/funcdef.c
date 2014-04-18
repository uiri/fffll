/* FFFLL - C implementation of a Fun Functioning Functional Little Language
   Copyright (C) 2013-2014 W Pearson

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

#include "funcdef.h"

extern List* varlist;

/* internal funcdef functions */

int incEachRefcountInTree(VarTree* vt) {
  if (vt == NULL)
    return 1;
  ((Value*)vt->data)->refcount++;
  incEachRefcountInTree(vt->left);
  incEachRefcountInTree(vt->right);
  return 0;
}

VarTree* insertEachKeywordInTree(VarTree* vt, VarTree* argt) {
  if (argt == NULL)
    return vt;
  if (argt->key[0] != '_' || findInTree(vt, argt->key) == NULL) {
    vt = insertInTree(vt, argt->key, argt->data);
  }
  vt = insertEachKeywordInTree(vt, argt->left);
  vt = insertEachKeywordInTree(vt, argt->right);
  return vt;
}

int descope(Value* v) {
  VarTree* vt;
  vt = varlist->data;
  freeEachValueInTree(vt, v);
  v->refcount--;
  freeTree(vt);
  varlist = deleteFromListBeginning(varlist);
  return 0;
}

int scope(FuncDef* fd, List* arglist) {
  int kw;
  VarTree* vt;
  List* fdname, *al;
  Value* v, *u;
  kw = 0;
  vt = NULL;
  if (fd->arguments != NULL && fd->arguments->data != NULL) {
    vt = copyTree(((List*)fd->arguments->data)->data);
    if (arglist != NULL && arglist->data != NULL)
      vt = insertEachKeywordInTree(vt, ((List*)arglist->data)->data);
    incEachRefcountInTree(vt);
  }
  if (fd->arguments == NULL || arglist == NULL) {
    addToListBeginning(varlist, vt);
    return 0;
  }
  fdname = fd->arguments->next;
  al = arglist->next;
  if (fdname == NULL && fd->arguments->data != NULL) {
    fdname = ((List*)fd->arguments->data)->next;
    kw = 1;
  }
  while (fdname != NULL && al != NULL) {
    v = evaluateValue(al->data);
    if (v == NULL) {
      addToListBeginning(varlist, vt);
      return 1;
    }
    v->refcount++;
    if (arglist->data) {
      while (fdname != NULL && findInTree(((List*)arglist->data)->data, ((Variable*)fdname->data)->name))
	fdname = fdname->next;
      if (fdname == NULL)
	break;
    }
    if (vt)
      vt = insertInTree(vt, ((Variable*)fdname->data)->name, v);
    else
      vt = newTree(((Variable*)fdname->data)->name, v);
    if (((Value*)al->data)->type == 'c') {
      u = ((Value*)al->data)->data;
      if (u->type == 'v' || u->type == 'c')
	u = evaluateValue(u);
      if (u->type == 'a' && ((FuncDef*)u->data)->alloc == 1) {
	freeValue(v);
      }
    }
    fdname = fdname->next;
    if (!kw && fdname == NULL && fd->arguments->data != NULL) {
      fdname = ((List*)fd->arguments->data)->next;
      kw = 1;
    }
    al = al->next;
  }
  addToListBeginning(varlist, vt);
  return 0;
}

/* external used by value.c */

Value* evaluateFuncDef(FuncDef* fd, List* arglist) {
  Value* val;
  if (scope(fd, arglist)) return NULL;
  val = evaluateStatements(fd->statements);
  if (val == NULL)
    return NULL;
  descope(val);
  return val;
}

int freeEachValueInTree(VarTree* vt, Value* v) {
  if (vt == NULL)
    return 1;
  if (vt->data != v)
    freeValue(vt->data);
  freeEachValueInTree(vt->left, v);
  freeEachValueInTree(vt->right, v);
  return 0;
}

int freeFuncDef(FuncDef* fd) {
  if (fd->arguments) {
    if (fd->arguments->data && ((List*)fd->arguments->data)->data) {
      freeEachValueInTree(((List*)fd->arguments->data)->data, NULL);
      freeTree(((List*)fd->arguments->data)->data);
      freeValueList(((List*)fd->arguments->data)->next);
      freeListNode(fd->arguments->data);
    }
    freeValueList(fd->arguments->next);
  }
  freeValueList(fd->statements);
  return 0;
}
