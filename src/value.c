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

#include <curl/curl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evaluate.h"
#include "tree.h"

extern Value* falsevalue;
extern int lenconstants;
extern char* constants;

extern List* varlist;
extern List* varnames;
extern List* stringlist;
extern VarTree* globalvars;

extern int strsize;
extern int funcnum;
extern Value* funcdeftable[15];

extern Value* stdfiles[3];
extern FILE** stdinp, **stdoutp, **stderrp;

/* Internal free functions */

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

int freeFuncVal(FuncVal* fv) {
  fv->refcount--;
  if (fv->refcount < 1) {
    if (fv->val->type == 'c')
      free(fv->name);
    freeValue(fv->val);
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
    deleteFromListData(stringlist, s);
    free(s->val);
    free(s);
  }
  return 0;
}

/* External functions */

String* addToStringList(String* s) {
  int i, j;
  String* n;
  List* node;
  i = strlen(s->val);
  j = -1;
  for (node=stringlist;node != NULL;node = node->next) {
    n = node->data;
    if (n != NULL && i == strlen(n->val)) {
      for (j=0;j<i;j++) {
	if (s->val[j] != n->val[j]) break;
      }
      if (j == i) {
	freeString(s);
	s = n;
	n->refcount++;
	break;
      }
    }
  }
  if (j != i) {
    addToListBeginning(stringlist, s);
  }
  return s;
}

int cleanupFffll(Value* v) {
  int i;
  /*List* node;*/
  for (i=0;i<funcnum;i++) {
    if (funcdeftable[i] != NULL) {
      globalvars = deleteDataInTree(globalvars, funcdeftable[i]);
      freeValue(funcdeftable[i]);
    }
  }
  /*freeValueList(parseTreeList->data);*/
  for (i=0;i<3;i++) {
    globalvars = deleteDataInTree(globalvars, stdfiles[i]);
    freeValue(stdfiles[i]);
  }
  if (v != NULL && v != falsevalue) {
    freeValue(v);
  }
  freeEachValueInTree(globalvars, v);
  freeTree(globalvars);
  freeList(varlist);
  /*for (node=varnames;node != NULL;node = node->next) {
    if (((char*)node->data) < constants || ((char*)node->data) > constants+lenconstants)
      free(node->data);
      }
      freeList(varnames);*/
  freeValue(falsevalue);
  free(stdinp);
  free(stdoutp);
  free(stderrp);
  free(constants);
  curl_global_cleanup();
  return 0;
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
  List* node;
  s = falsevalue;
  for (node=sl;node != NULL;node = node->next) {
    t = node->data;
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

int freeRange(Range* range) {
  range->refcount--;
  if (range->refcount < 1) {
    freeValue(range->start);
    freeValue(range->end);
    freeValue(range->increment);
    freeValue(range->computed);
    free(range);
  }
  return 0;
}

int freeValue(Value* val) {
  if (val == NULL)
    return 1;
  val->refcount--;
  if (val->refcount < 1) {
    switch (val->type) {
    case 'n':
      free(val->data);
      break;
    case 'l':
      if (val->data && ((List*)val->data)->data) {
	freeEachValueInTree(((List*)((List*)val->data)->data)->data, NULL);
	freeTree(((List*)((List*)val->data)->data)->data);
	/*freeValueList(((List*)((List*)val->data)->data)->next);*/
	freeListNode(((List*)val->data)->data);
      }
      if (val->data)
	val->data = ((List*)val->data)->next;
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
    case 'a':
      freeFuncDef(val->data);
      return 1;
    case 'h':
      freeHttpVal((HttpVal*)val);
      return 1;
    case 'b':
      freeBoolExpr((BoolExpr*)val);
      return 1;
    case 'c':
      freeFuncVal((FuncVal*)val);
      return 1;
    case 'r':
      freeRange((Range*)val);
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
  List* node;
  if (r == NULL) {
    return 0;
  }
  for (node = r;node != NULL;node = node->next) {
    freeValue(node->data);
  }
  freeList(r);
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

FuncDef* newBuiltinFuncDef(Value* (*evaluate)(FuncDef*, List*), int alloc) {
  FuncDef* fd;
  fd = newFuncDef(NULL, NULL, alloc);
  fd->evaluate = evaluate;
  return fd;
}

FuncDef* newFuncDef(List* al, List* sl, int alloc) {
  FuncDef* fd;
  fd = malloc(sizeof(FuncDef));
  fd->statements = sl;
  fd->arguments = al;
  fd->evaluate = &evaluateFuncDef;
  fd->alloc = alloc;
  return fd;
}

FuncVal* newFuncVal(Value* val, List* arglist, char* name, int ln) {
  FuncVal* fv;
  fv = malloc(sizeof(FuncVal));
  fv->refcount = 1;
  fv->val = val;
  fv->type = 'c';
  fv->arglist = arglist;
  fv->name = name;
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

Range* newRange(Value* start, Value* end, Value* increment) {
  Range* range;
  if (increment == NULL) {
    increment = falsevalue;
  }
  range = malloc(sizeof(Range));
  range->type = 'r';
  range->refcount = 1;
  range->computed = NULL;
  range->start = start;
  range->end = end;
  range->increment = increment;
  return range;
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

Variable* parseVariable(char* name) {
  int i, j;
  char* n;
  List* node;
  for (i=0;name[i] != '\0';i++);
  j = 0;
  for (node=varnames;node != NULL;node = node->next) {
    n = node->data;
    if (n == NULL)
      continue;
    if (i == strlen(n)) {
      for (j=0;j<i;j++) {
        if (name[j] != n[j]) break;
      }
      if (j == i) {
        free(name);
        name = n;
        break;
      }
    }
  }
  if (j != i) {
    addToListBeginning(varnames, name);
  }
  return newVariable(name);
}
