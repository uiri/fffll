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

#include <curl/curl.h>
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

/* Internal free functions */

int freeFuncVal(FuncVal* fv) {
  fv->refcount--;
  if (fv->refcount < 1) {
    free(fv->name);
    freeValueList(fv->arglist);
    free(fv);
  }
  return 0;
}

int freeHttpVal(HttpVal* hv) {
  hv->refcount--;
  if (hv->refcount < 1) {
    curl_easy_cleanup(hv->curl);
    free(hv->buf);
    free(hv->url);
    free(hv);
  }
  return 0;
}

int freeString(String* s) {
  s->refcount--;
  if (s->refcount < 1) {
    free(s->val);
    free(s);
    deleteFromListData(stringlist, s);
  }
  return 0;
}

int freeVariable(Variable* var) {
  int i;
  var->refcount--;
  if (var->refcount < 1) {
    for (i=0;var->indextype[i] != '0';i++) {
      if (var->indextype[i] == 'n')
	free(var->index[i]);
      else if (var->indextype[i] != '0')
	freeValue(var->index[i]);
    }
    free(var->indextype);
    free(var->index);
    free(var);
  }
  return 0;
}

/* External functions */

String* addToStringList(String* s) {
  int i, j, k, l;
  String* n;
  l = lengthOfList(stringlist);
  k = strlen(s->val);
  j = -1;
  for (i=0;i<l;i++) {
    n = dataInListAtPosition(stringlist, i);
    if (n != NULL && k == strlen(n->val)) {
      for (j=0;j<k;j++) {
	if (s->val[j] != n->val[j]) break;
      }
      if (j == k) {
	freeString(s);
	s = n;
	n->refcount++;
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

int freeValue(Value* val) {
  val->refcount--;
  if (val->refcount < 1) {
    switch (val->type) {
    case 'n':
      free(val->data);
      break;
    case 'l':
    case 'd':
      freeValueList(val->data);
      break;
    case 's':
      freeString(val->data);
      break;
    case 'f':
      if (*(FILE**)val->data != stdin && *(FILE**)val->data != stdout
	  && *(FILE**)val->data != stderr) {
	fclose(*(FILE**)val->data);
	free(val->data);
      }
      break;
    case 'h':
      freeHttpVal((HttpVal*)val);
      return 1;
    case 'b':
      freeBoolExpr((BoolExpr*)val);
      return 1;
    case 'c':
      freeFuncVal((FuncVal*)val);
      return 1;
    case 'v':
      freeVariable((Variable*)val);
      return 1;
    }
    free(val);
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

int hashName(char* name) {
  int i, s;
  s = 0;
  /* Hash from sdbm. See http://www.cse.yorku.ca/~oz/hash.html */
  for (i=0;name[i] != '\0';i++)
    s = name[i] + (s << 6) + (s << 16);
  /* funcnum is always 2^n - 1 */
  s = s&funcnum;
  return s;
}

int insertFunction(FuncDef* fd) {
  int i;
  i = hashName(fd->name);
  while (funcdeftable[i] != NULL) {
    if (funcdeftable[i]->name == fd->name) {
      free(funcdeftable[i]);
      break;
    }
    i++;
  }
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

HttpVal* newHttpVal(char* url) {
  HttpVal* hv;
  hv = malloc(sizeof(HttpVal));
  hv->url = url;
  hv->refcount = 1;
  hv->type = 'h';
  hv->curl = curl_easy_init();
  curl_easy_setopt(hv->curl, CURLOPT_URL, url);
  curl_easy_setopt(hv->curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(hv->curl, CURLOPT_MAXREDIRS, 20L);
  curl_easy_setopt(hv->curl, CURLOPT_AUTOREFERER, 1L);
  curl_easy_setopt(hv->curl, CURLOPT_USERAGENT, "FFFLL");
  curl_easy_setopt(hv->curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(hv->curl, CURLOPT_SSL_VERIFYHOST, 0L);
  hv->buf = NULL;
  hv->pos = 0;
  hv->bufsize = 0;
  return hv;
}

String* newString(char* s) {
  String* str;
  str = malloc(sizeof(String));
  str->val = s;
  str->refcount = 1;
  return str;
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
  var->indextype = malloc(1);
  var->index = malloc(sizeof(void*));
  var->indextype[0] = '0';
  var->index[0] = NULL;
  return var;
}
