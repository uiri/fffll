/* FFFLL - C implementation of a Fun Functioning Functional Little Language
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evaluate.h"
#include "tree.h"

extern Value* falsevalue;

extern List* varlist;
extern List* stringlist;

extern int funcnum;
extern FuncDef** funcdeftable;

char* addToStringList(char* s, int freestr) {
  int i, j, k, l;
  char* n;
  l = lengthOfList(stringlist);
  k = strlen(s);
  j = -1;
  for (i=0;i<l;i++) {
    n = dataInListAtPosition(stringlist, i);
    if (n != NULL && k == strlen(n)) {
      for (j=0;j<k;j++) {
	if (s[j] != n[j]) break;
      }
      if (j == k) {
	if (freestr)
	  free(s);
	s = n;
	break;
      }
    }
  }
  if (j != k) {
    addToListBeginning(stringlist, s);
  }
  return s;
}

int errmsg(char* err) {
  fprintf(stderr, "%s\n", err);
  return 0;
}

int errmsgf(char* format, char* s) {
  char* err;
  err = malloc(strlen(format) + strlen(s) + 2);
  sprintf(err, format, s);
  errmsg(err);
  free(err);
  return 0;
}

int errmsgfd(char* format, char* s, int i) {
  char* err;
  err = malloc(strlen(format) + strlen(s) + (i/10) + 3);
  sprintf(err, format, s, i);
  errmsg(err);
  free(err);
  return 0;
}

Value* evaluateStatements(List* sl) {
  Value* s, *t;
  int i,l;
  l = lengthOfList(sl);
  s = falsevalue;
  for (i=0;i<l;i++) {
    t = dataInListAtPosition(sl, i);
    t = evaluateValue(t);
    if (t == NULL) {
      return NULL;
    }
    if (t != falsevalue)
      s = t;
  }
  return s;
}

int freeBoolExpr(BoolExpr* be) {
  be->refcount--;
  if (be->refcount < 1) {
    freeValueList(be->stack);
    free(be);
  }
  return 0;
}

int freeFuncVal(FuncVal* fv) {
  fv->refcount--;
  if (fv->refcount < 1) {
    free(fv->name);
    freeValueList(fv->arglist);
    free(fv);
    return 1;
  }
  return 0;
}

int freeValue(Value* val) {
  val->refcount--;
  if (val->refcount < 1) {
    if (val->type == 'n') {
      free(val->data);
    }
    if (val->type == 'f') {
      if (*(FILE**)val->data != stdin && *(FILE**)val->data != stdout
	  && *(FILE**)val->data != stderr) {
	fclose(*(FILE**)val->data);
	free(val->data);
      }
    }
    if (val->type == 'b') {
      freeBoolExpr((BoolExpr*)val);
      return 1;
    }
    if (val->type == 'c') {
      freeFuncVal((FuncVal*)val);
      return 1;
    }
    if (val->type == 'l' || val->type == 'd') {
      freeValueList(val->data);
    }
    if (val->type == 'v') {
      freeVariable((Variable*)val);
      return 1;
    }
    free(val);
    return 1;
  }
  return 0;
}

int freeValueList(List* r) {
  int i, l;
  l = lengthOfList(r);
  for (i=0;i<l;i++) {
    freeValue(dataInListAtPosition(r, i));
  }
  freeList(r);
  return 0;
}

int freeVariable(Variable* var) {
  var->refcount--;
  if (var->refcount < 1) {
    free(var);
    return 1;
  }
  return 0;
}

FuncDef* getFunction(char* name) {
  FuncDef* fd;
  int i;
  i = hashName(name);
  fd = NULL;
  while (i<funcnum) {
    fd = funcdeftable[i++];
    if (fd == NULL) {
      errmsgf("Function %s is not defined", name);
      break;
    }
    if (fd->name == name) break;
  }
  return fd;
}

int hashName(char* name) {
  int i, s;
  s = 0;
  /* Hash from sdbm. See http://www.cse.yorku.ca/~oz/hash.html */
  for (i=0;name[i] != '\0';i++) {
    s += name[i] + (s << 6) + (s << 16) - s;
    /* funcnum is always 2^n - 1 */
    s = s&funcnum;
  }
  return s;
}

int insertFunction(FuncDef* fd) {
  int i;
  i = hashName(fd->name);
  while (funcdeftable[i] != NULL)
    i++;
  funcdeftable[i] = fd;
  return 0;
}

BoolExpr* newBoolExpr(Value* val) {
  BoolExpr* be;
  be = malloc(sizeof(BoolExpr));
  be->refcount = 1;
  be->stack = newList();
  be->type = 'b';
  be->neg = 0;
  be->lasteval = -1;
  addToListEnd(be->stack, val);
  return be;
}

int newBuiltinFuncDef(char* name, Value* (*evaluate)(FuncDef*, List*), int alloc) {
  FuncDef* fd;
  fd = newFuncDef(name, NULL, NULL, alloc);
  fd->evaluate = evaluate;
  return insertFunction(fd);
}

FuncDef* newFuncDef(char* name, List* al, List* sl, int alloc) {
  FuncDef* fd;
  fd = malloc(sizeof(FuncDef));
  fd->name = name;
  fd->statements = sl;
  fd->arguments = al;
  fd->evaluate = &evaluateFuncDef;
  fd->alloc = alloc;
  return fd;
}

FuncVal* newFuncVal(char* name, List* arglist, int ln) {
  FuncVal* fv;
  fv = malloc(sizeof(FuncVal));
  fv->refcount = 1;
  fv->name = name;
  fv->type = 'c';
  fv->arglist = arglist;
  fv->lineno = ln;
  return fv;
}

Value* newValue(char type, void* data) {
  Value* val;
  val = malloc(sizeof(Value));
  val->refcount = 1;
  val->type = type;
  val->data = data;
  return val;
}

Variable* newVariable(char* name) {
  Variable* var;
  var = malloc(sizeof(Variable));
  var->refcount = 1;
  var->name = name;
  var->type = 'v';
  return var;
}

Value* valueFromName(char* name) {
  Value* v;
  v = findInTree(varlist->data, name);
  if (v == NULL) {
    errmsgf("Variable named '%s' is not SET", name);
    return NULL;
  }
  return v;
}
