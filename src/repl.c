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

#include <stdlib.h>
#include <string.h>
#include "evaluate.h"
#include "typecast.h"

extern Value* falsevalue;

/* Internal errmsg helper */

int errmsgfd(char* format, char* s, int i) {
  char* err;
  err = malloc(strlen(format) + strlen(s) + (i/10) + 3);
  sprintf(err, format, s, i);
  errmsg(err);
  free(err);
  return 0;
}

/* External repl functions */

Value* evaluateFuncVal(FuncVal* fv) {
  FuncDef* fd;
  Value* v, *u;
  v = fv->val;
  u = NULL;
  fd = NULL;
  if (v->type != 'a') {
    u = v;
    v = evaluateValue(u);
  }
  if (v == NULL || v->type != 'a') {
    errmsgf("Attempt to use non-function '%s' as a functor", fv->name);
    v = NULL;
  }
  if (v != NULL) {
    fd = v->data;
  }
  if (fd == NULL || (v = (*fd->evaluate)(fd, fv->arglist)) == NULL) {
    errmsgfd("In %s at line %d", fv->name, fv->lineno);
    return NULL;
  }
  return v;
}

char* valueToString(Value* v) {
  char* s, *t;
  int i, j, l, freet;
  double a, b, c;
  BoolExpr* be;
  Value* u;
  List* node;
  freet = 0;
  l = 0;
  t = NULL;
  if (v->type == 'v') {
    u = evaluateValue(v);
    if (u == NULL) return NULL;
    s = valueToString(u);
    if (s == NULL) return NULL;
    return s;
  }
  if (v->type == 'i') {
    u = evaluateValue(v);
    if (u == NULL) return NULL;
    s = valueToString(u);
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
  if (v->type == 'r') {
    a = valueToDouble(((Range*)v)->start);
    c = valueToDouble(((Range*)v)->end);
    if (((Range*)v)->increment == falsevalue) {
      b = 1.0;
      if (c<a)
	b = -1.0;
    } else {
      b = valueToDouble(((Range*)v)->increment);
    }
    s = malloc(1);
    if ((c-a)/b < 0) {
      s[0] = '\0';
      return s;
    }
    l = 1;
    if (a<c) {
      for (;a<c;a += b) {
	l += snprintf(s, 0, "%d, ", (int)a);
      }
    } else if (c<a) {
      for (;c<a;a += b) {
	l += snprintf(s, 0, "%d, ", (int)a);
      }
    }
    free(s);
    s = malloc(l);
    a = valueToDouble(((Range*)v)->start);
    i = 0;
    if (a<c) {
      for (;a<c;a += b) {
	  i += snprintf(s+i, l, "%d, ", (int)a);
      }
    } else if (c<a) {
      for (;c<a;a += b) {
	i += snprintf(s+i, l-i, "%d, ", (int)a);
      }
    }
    i--;
    s[--i] = '\0';
    return s;
  }
  if (v->type == 'l') {
    s = malloc(4);
    s[0] = '[';
    i = 0;
    node = NULL;
    if (v->data)
      node = ((List*)v->data)->next;
    for (;node != NULL;node = node->next) {
      if (i) s[i] = ' ';
      i++;
      u = evaluateValue(node->data);
      if (u == NULL) {
	free(s);
	return NULL;
      }
      t = valueToString(u);
      freeValue(u);
      if (t == NULL) {
	free(s);
	return NULL;
      }
      l = strlen(t);
      s = realloc(s, i+l+2);
      for (j=0;j<l;j++) {
	s[i+j] = t[j];
      }
      s[i+++j] = ',';
      s[i+j] = '\0';
      i += l;
      free(t);
    }
    if (!i) i = 2;
    s[--i] = ']';
    s[++i] = '\0';
    return s;
  }
  if (v->type == 'c') {
    u = evaluateFuncVal((FuncVal*)v);
    if (u == NULL) return NULL;
    s = valueToString(u);
    freeValue(u);
    return s;
  }
  if (v->type == 'd') {
    u = evaluateStatements(v->data);
    if (u == NULL) return NULL;
    s = valueToString(u);
    freeValue(u);
    return s;
  }
  if (v->type == 's') {
    t = ((String*)v->data)->val;
  }
  if (t == NULL) return NULL;
  l = strlen(t);
  s = malloc(l+1);
  for (i=0;i<l;i++) {
    s[i] = t[i];
  }
  s[i] = '\0';
  if (freet) free(t);
  return s;
}
