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
#include "list.h"
#include "tree.h"
#include "value.h"

extern Value* falsevalue;
extern List* varlist;
extern List* stringlist;
extern List* funcnames;

/* Internal helper functions */

double valueToDouble(Value* v) {
  double d, n;
  int i, l;
  BoolExpr* be;
  Value* u;
  n = 0.0;
  if (v->type == 'n')
    return *(double*)v->data;
  if (v->type == 's')
    return atof((char*)v->data);
  if (v->type == 'b') {
    be = evaluateBoolExpr((BoolExpr*)v);
    if (be == NULL) return NAN;
    return (double)(be->lasteval);
  }
  if (v->type == 'c') {
    u = (Value*)evaluateFuncVal((FuncVal*)v);
    if (u == NULL) return NAN;
    d = valueToDouble(u);
    freeValue(u);
    return d;
  }
  if (v->type == 'd')
    return valueToDouble(evaluateStatements(v->data));
  if (v->type == 'l') {
    l = lengthOfList(v->data);
    for (i=0;i<l;i++) {
      u = evaluateValue(dataInListAtPosition(v->data, i));
      d = valueToDouble(u);
      freeValue(u);
      if (d < 0) return d;
      n += d;
    }
    return n;
  }
  if (v->type == 'v') {
    u = evaluateValue(v);
    if (u == NULL) return NAN;
    n = valueToDouble(u);
    freeValue(u);
  }
  return n;
}

char* valueToString(Value* v) {
  char* s, *t;
  int i, j, k, l, freet, m;
  BoolExpr* be;
  Value* u;
  freet = 0;
  l = 0;
  t = NULL;
  if (v->type == 'v') {
    u = valueFromName(((Variable*)v)->name);
    if (u == NULL) return NULL;
    s = valueToString(u);
    if (s == NULL) return NULL;
    return s;
  }
  if (v->type == 'n') {
    l = 32;
    i = l;
    s = malloc(l);
    l = snprintf(s, l, "%g", *(double*)v->data);
    if (i<l) {
      s = realloc(s, l);
      snprintf(s, l, "%g", *(double*)v->data);
    }
    return s;
  }
  if (v->type == '0' || v->type == 'b') {
    if (v->type == 'b') {
      be = evaluateBoolExpr((BoolExpr*)v);
      if (be == NULL) return NULL;
      l = be->lasteval;
    }
    if (l) {
      s = malloc(5);
      s[0] = 't';
      s[1] = 'r';
      s[2] = 'u';
      s[3] = 'e';
      s[4] = '\0';
    } else {
      s = malloc(6);
      s[0] = 'f';
      s[1] = 'a';
      s[2] = 'l';
      s[3] = 's';
      s[4] = 'e';
      s[5] = '\0';
    }
    return s;
  }
  if (v->type == 'l') {
    l = lengthOfList(v->data);
    s = malloc(4);
    s[0] = '[';
    k = 0;
    for (i=0;i<l;i++) {
      if (k) s[k] = ' ';
      k++;
      u = evaluateValue(dataInListAtPosition(v->data, i));
      if (u == NULL) return NULL;
      t = valueToString(u);
      freeValue(u);
      if (t == NULL) return NULL;
      m = strlen(t);
      s = realloc(s, k+m+2);
      for (j=0;j<m;j++) {
	s[k+j] = t[j];
      }
      s[k+++j] = ',';
      s[k+j] = '\0';
      k += m;
      free(t);
    }
    if (!k) k = 2;
    s[--k] = ']';
    s[++k] = '\0';
    return s;
  }
  if (v->type == 'c') {
    freet = 1;
    u = evaluateFuncVal((FuncVal*)v);
    if (u == NULL) return NULL;
    t = valueToString(u);
    freeValue(u);
    if (t == NULL) return NULL;
  }
  if (v->type == 'd') {
    freet = 1;
    u = evaluateStatements(v->data);
    if (u == NULL) return NULL;
    t = valueToString(u);
    freeValue(u);
    if (t == NULL) return NULL;
  }
  if (v->type == 's') {
    t = v->data;
  }
  l = strlen(t);
  s = malloc(l+1);
  for (i=0;i<l;i++) {
    s[i] = t[i];
  }
  s[i] = '\0';
  if (freet) free(t);
  return s;
}

/* External Function Definitions */

Value* addDef(FuncDef* fd, List* arglist) {
  double d, *n;
  int i,l;
  arglist = evaluateList(arglist);
  if (arglist == NULL)
    return NULL;
  l = lengthOfList(arglist);
  n = malloc(sizeof(double));
  *n = 0.0;
  for (i=0;i<l;i++) {
    d = valueToDouble(dataInListAtPosition(arglist, i));
    *n += d;
  }
  freeValueList(arglist);
  if (isnan(*n)) {
    free(n);
    return NULL;
  }
  return newValue('n', n);
}

Value* catDef(FuncDef* fd, List* arglist) {
  char* s, *t;
  int i, j, k, l;
  arglist = evaluateList(arglist);
  if (arglist == NULL) {
    printf("CAT requires at least one argument");
    return NULL;
  }
  l = lengthOfList(arglist);
  k = 0;
  s = NULL;
  for (i=0;i<l;i++) {
    t = valueToString(dataInListAtPosition(arglist, i));
    j = strlen(t);
    k += j;
    s = realloc(s, k+1);
    strncat(s, t, j);
    s[k] = '\0';
    free(t);
  }
  s = addToStringList(s, 1);
  return newValue('s', s);
}

Value* defDef(FuncDef* fd, List* arglist) {
  char* name, *fn;
  int i, j, k, l;
  l = lengthOfList(arglist);
  if (l < 3) {
    printf("Not enough arguments for DEF\n");
    return NULL;
  }
  name = valueToString(arglist->data);
  l = strlen(name);
  for (i=0;i<l;i++) {
    if (name[i] > 90) {
      name[i] -= 32;
    }
  }
  l = lengthOfList(funcnames);
  k = strlen(name);
  j = -1;
  for (i=0;i<l;i++) {
    fn = dataInListAtPosition(funcnames, i);
    if (fn != NULL && strlen(fn) == k) {
      for (j=0;j<k;j++) {
	if (fn[j] != name[j]) break;
      }
      if (j == k) break;
    }
  }
  if (j == k) {
    free(name);
    name = fn;
  }
  insertFunction(newFuncDef(name, ((Value*)arglist->next->data)->data,
			    ((Value*)arglist->next->next->data)->data));
  ((Value*)arglist->data)->refcount++;
  return (Value*)arglist->data;
}

Value* headDef(FuncDef* fd, List* arglist) {
  Value* v;
  if (lengthOfList(arglist) != 1) {
    printf("HEAD only takes one argument.");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v->type != 'l') {
    printf("HEAD's argument must be a list.");
    return NULL;
  }
  return evaluateValue(((List*)v->data)->data);
}

Value* ifDef(FuncDef* fd, List* arglist) {
  int l;
  BoolExpr* be;
  l = lengthOfList(arglist);
  if (l < 2) {
    printf("Not enough arguments for IF\n");
    return NULL;
  }
  be = evaluateBoolExpr(arglist->data);
  if (be == NULL) return NULL;
  if (be->lasteval) {
    return evaluateStatements((List*)((Value*)arglist->next->data)->data);
  }
  if (l > 2) {
    arglist = arglist->next;
    return evaluateStatements((List*)((Value*)arglist->next->data)->data);
  }
  return falsevalue;
}

Value* lenDef(FuncDef* fd, List* arglist) {
  double* a;
  int i, l;
  Value* v;
  if (arglist == NULL) {
    printf("Not enough arguments for LEN\n");
  }
  arglist = evaluateList(arglist);
  if (arglist == NULL)
    return NULL;
  a = malloc(sizeof(double));
  *a = 0.0;
  l = lengthOfList(arglist);
  for (i=0;i<l;i++) {
    v = dataInListAtPosition(arglist, i);
    if (v->type == 's') {
      *a += (double)strlen(v->data);
    } else if (v->type == 'l' || v->type == 'd') {
      *a += (double)lengthOfList(v->data);
    } else if (v->type != '0') {
      printf("LEN only takes variables, function calls, strings, lists or blocks as arguments\n");
      freeValueList(arglist);
      free(a);
      return NULL;
    }
  }
  return newValue('n', a);
}

Value* mulDef(FuncDef* fd, List* arglist) {
  double d, *n;
  int i,l;
  arglist = evaluateList(arglist);
  if (arglist == NULL)
    return NULL;
  l = lengthOfList(arglist);
  n = malloc(sizeof(double));
  *n = 1.0;
  for (i=0;i<l;i++) {
    d = valueToDouble(dataInListAtPosition(arglist, i));
    if (isnan(d)) {
      freeValueList(arglist);
      free(n);
      return NULL;
    }
    *n *= d;
  }
  freeValueList(arglist);
  return newValue('n', n);
}

Value* openDef(FuncDef* fd, List* arglist) {
  FILE** fp;
  Value* v;
  char* s;
  if (arglist == NULL) {
    printf("Not enough arguments for OPEN\n");
  }
  v = evaluateValue(arglist->data);
  if (v == NULL) return NULL;
  s = valueToString(v);
  fp = malloc(sizeof(FILE*));
  *fp = fopen(s, "a+");
  free(s);
  return newValue('f', fp);
}

Value* rcpDef(FuncDef* fd, List* arglist) {
  double b, d, *n;
  int i, e;
  if (lengthOfList(arglist) != 1) {
    printf("RCP only takes 1 argument, the number whose reciprocal"
	   " is to be computed.\n");
    return NULL;
  }
  d = valueToDouble(arglist->data);
  if (isnan(d))
    return NULL;
  n = malloc(sizeof(double));
  *n = frexp(d, &e);
  e -= 1;
  for (i=0;i<e;i++) {
    *n *= 0.5;
  }
  for (i=0;i<4;i++) {
    b = *n * (-1.0);
    *n = fma(d, b, 2);
    *n *= (-1.0)*b;
  }
  return newValue('n', n);
}

Value* readDef(FuncDef* fd, List* arglist) {
  char c;
  char* s;
  FILE* fp;
  double n, l;
  int i;
  Value* v;
  if (arglist == NULL) {
    printf("Not enough arguments for READ\n");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v == NULL) return NULL;
  if (v->type != 'f') {
    printf("READ only takes a file as its argument.\n");
    return NULL;
  }
  fp = *(FILE**)v->data;
  if (feof(fp)) {
    printf("Attempt to READ past the end of a file.\n");
    return NULL;
  }
  n = 0;
  if (fp != stdin && fp != stdout && fp != stderr &&
      lengthOfList(arglist) > 1) {
    v = evaluateValue(arglist->next->data);
    if (v->type == 'n') {
      n = *(double*)v->data;
      l = n;
      if (n < 0) {
	for (i=-1;l<0;l++) {
	  do {
	    i--;
	    fseek(fp, i, SEEK_END);
	    c = fgetc(fp);
	  } while (c != '\n');
	}
      } else {
	fseek(fp, 0, SEEK_SET);
	for (i=0;i<l;i++) {
	  for (c=fgetc(fp);c != '\n';c=fgetc(fp));
	  if (c == EOF) break;
	}
      }
    }
  }
  l = 32;
  s = malloc(l);
  i = 0;
  for (c=fgetc(fp);c != '\n' && c != EOF;c=fgetc(fp)) {
    if (i+1 > l) {
      l *= 2;
      s = realloc(s, l);
    }
    s[i++] = c;
  }
  s[i] = '\0';
  if (n == -1) {
    fseek(fp, 0, SEEK_SET);
  }
  fseek(fp, 0, SEEK_CUR);
  s = addToStringList(s, 1);
  return newValue('s', s);
}

Value* retDef(FuncDef* fd, List* arglist) {
  Value* v;
  if (arglist == NULL) {
    falsevalue->refcount++;
    return falsevalue;
  }
  if (((Value*)arglist->data)->type != 'd') {
    printf("RETURN only takes a statement block as its argument.\n");
    return NULL;
  }
  v = evaluateStatements(((Value*)arglist->data)->data);
  v->refcount++;
  return v;
}

Value* setDef(FuncDef* fd, List* arglist) {
  Value* v, *u;
  if (lengthOfList(arglist) < 2) {
    printf("Not enough arguments for SET\n");
    return NULL;
  }
  if (((Value*)arglist->data)->type != 'v') {
    printf("SET requires a variable to be its first argument.\n");
    return NULL;
  }
  v = evaluateValue(arglist->next->data);
  if (v == NULL) {
    return NULL;
  }
  u = findInTree(varlist->data, ((Variable*)arglist->data)->name);
  if (u != NULL) {
    freeValue(u);
  }
  varlist->data = insertInTree(varlist->data, ((Variable*)arglist->data)->name, v);
  return v;
}

Value* tailDef(FuncDef* fd, List* arglist) {
  Value* v;
  List* ll, *tl;
  if (lengthOfList(arglist) != 1) {
    printf("TAIL only takes one argument.");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v->type != 'l') {
    printf("TAIL's argument must be a list.");
    return NULL;
  }
  ll = ((List*)v->data)->next;
  tl = ll;
  while (tl != NULL) {
    ((Value*)tl->data)->refcount++;
    tl = tl->next;
  }
  return newValue('l', ll);
}

Value* tokDef(FuncDef* fd, List* arglist) {
  List* l;
  Value* v;
  char* s, *t, *r;
  int g, h, i, j, k;
  if (lengthOfList(arglist) != 2) {
    printf("TOK takes exactly two arguments, a string to tokenize and a delimiter\n");
    return NULL;
  }
  arglist = evaluateList(arglist);
  if (arglist == NULL)
    return NULL;
  l = newList();
  if (((Value*)arglist->data)->type != 's' || ((Value*)arglist->next->data)->type != 's') {
    printf("The arguments to TOK must be strings.\n");
    return NULL;
  }
  r = ((Value*)arglist->data)->data;
  t = ((Value*)arglist->next->data)->data;
  h = strlen(r)+1;
  k = strlen(t);
  s = malloc(h);
  g = 0;
  for (i=0;i<h;i++) {
    for (j=0;(i+j)<h&&j<k;j++) {
      if (r[i+j] != t[j])
	break;
    }
    if (j != k)
      continue;
    j = g;
    for (;g<i;g++) {
      s[g] = r[g];
    }
    s[i] = '\0';
    v = newValue('s', addToStringList(s+j, 0));
    addToListEnd(l, v);
    g += k;
  }
  if (l->next == NULL)
    free(s);
  else
    addToListEnd(stringlist, s);
  freeValueList(arglist);
  return newValue('l', l);
}

Value* whileDef(FuncDef* fd, List* arglist) {
  Value* v;
  BoolExpr* be;
  if (lengthOfList(arglist) < 2) {
    printf("Not enough arguments for WHILE\n");
    return NULL;
  }
  v = falsevalue;
  v->refcount++;
  for (be = evaluateBoolExpr(arglist->data);
       be != NULL && be->lasteval;
       be = evaluateBoolExpr(arglist->data)) {
    v = evaluateStatements((List*)((Value*)arglist->next->data)->data);
    if (v == NULL) return NULL;
  }
  if (be == NULL) return NULL;
  return v;
}

Value* writeDef(FuncDef* fd, List* arglist) {
  char* s;
  int i, j, k, l, m;
  FILE* fp;
  Value* v;
  if (arglist == NULL) {
    printf("Not enough arguments for WRITE\n");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v == NULL) return NULL;
  if (v->type != 'f') {
    printf("WRITE only takes a file as its first argument.\n");
    return NULL;
  }
  fp = *(FILE**)v->data;
  l = lengthOfList(arglist);
  m = 0;
  if (fp == stdout || fp == stderr) {
    m = 1;
  }
  for (i=1;i<l;i++) {
    s = valueToString(dataInListAtPosition(arglist, i));
    if (s == NULL) {
      return NULL;
    }
    k = strlen(s);
    for (j=0;j<k;j++) {
      fputc(s[j], fp);
    }
    free(s);
  }
  if (m || j == -1)
    fputc('\n', fp);
  fseek(fp, 0, SEEK_CUR);
  return falsevalue;
}
