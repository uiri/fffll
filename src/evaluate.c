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

#include <math.h>
#include <pcre.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evaluate.h"

extern List* varlist;
extern VarTree* globalvars;
extern List* varnames;

extern int funcnum;
extern FuncDef** funcdeftable;

extern Value* falsevalue;

/* External evaluation functions */

BoolExpr* evaluateBoolExpr(BoolExpr* be) {
  int m, p, erroff, ovec[PCREOVECCOUNT];
  double j, k, **n, *o;
  char* c, *s, *t;
  const char* err;
  List* stack, *prop, *stackiter;
  Value* v, *u, *w;
  pcre *re;
  re = NULL;
  stack = be->stack;
  for (m=0;stack->next && stack->next->next;m++)
    stack = stack->next->next;
  n = malloc((m+2)*sizeof(double*));
  m = -1;
  stack = newList();
  o = NULL;
  stackiter = be->stack;
  while (stackiter) {
    s = NULL;
    t = NULL;
    w = NULL;
    v = NULL;
    u = stackiter->data;
    if (u->type == '|' || u->type == '&') {
      addToListEnd(stack, n[m]);
      addToListEnd(stack, u);
      stackiter = stackiter->next;
      continue;
    }
    n[++m] = calloc(1, sizeof(double));
    if (u->type != 'b') {
      v = evaluateValue(u);
      w = v;
      if (v == NULL) {
	freeList(stack);
	m++;
	for (p=0;p<m;p++) {
	  free(n[p]);
	}
	free(n);
	return NULL;
      }
      if (v->type == 's') {
	s = v->data;
      }
      j = evaluateValueAsBool(v);
      if (u->type != 'v')
	freeValue(v);
    } else {
      j = evaluateValueAsBool(u);
    }
    if (isnan(j)) {
      m++;
      for (p=0;p<m;p++) {
	free(n[p]);
      }
      free(n);
      freeList(stack);
      return NULL;
    }
    stackiter = stackiter->next;
    if (j) *n[m] = 1;
    if (!stackiter) {
      break;
    }
    c = (char*)stackiter->data;
    stackiter = stackiter->next;
    if (c[0] == '|' || c[0] == '&') {
      addToListEnd(stack, n[m]);
      addToListEnd(stack, c);
      continue;
    }
    n[++m] = calloc(1, sizeof(double));
    u = stackiter->data;
    if (u->type != 'b') {
      v = evaluateValue(u);
      if (v == NULL) {
	freeList(stack);
	m++;
	for (p=0;p<m;p++) {
	  free(n[p]);
	}
	free(n);
	return NULL;
      }
      if (v->type == 's') {
	t = v->data;
      }
      if (u->type != 'v')
	freeValue(v);
    }
    k = evaluateValueAsBool(stackiter->data);
    if (isnan(k)) {
      m++;
      for (p=0;p<m;p++) {
	free(n[p]);
      }
      free(n);
      freeList(stack);
      return NULL;
    }
    stackiter = stackiter->next;
    if (c[0] == '~') {
      if (w == NULL) {
	continue;
      }
      s = valueToString(w);
      t = valueToString(v);
      re = pcre_compile(t, 0, &err, &erroff, NULL);
      if (pcre_exec(re, NULL, s, strlen(s), 0, 0, ovec, PCREOVECCOUNT) > -1)
	*n[m] = 1;
      free(s);
      free(t);
      if (re) pcre_free(re);
      continue;
    }
    if (c[0] == '?') {
      if (w == NULL || v == NULL)
	continue;
      if (w->type == v->type) {
	if (v->type != 'l' || ((List*)v->data)->data == NULL) {
	  *n[m] = 1;
	  continue;
	}
	prop = ((List*)((List*)v->data)->data)->next;
	while (prop != NULL) {
	  if (!findInTree(((List*)((List*)w->data)->data)->data, ((Variable*)prop->data)->name))
	    break;
	  prop = prop->next;
	}
	if (prop == NULL)
	  *n[m] = 1;
      }
    }
    if (t != NULL && s != NULL) {
      if (s == t && c[0] == '=') {
	  *n[m] = 1;
	  continue;
      }
      for (p=0;s[p] != '\0' && s[p] == t[p];p++);
      if ((c[0] == '<' && s[p] < t[p]) || (c[0] == '>' && s[p] > t[p]))
	*n[m] = 1;
      continue;
    }
    p = 0;
    if (j>k) {
      if (j<=0.0) {
	if (fabs((j-k)/k)>=0.00000000005)
	  p = 1;
      } else {
	if (fabs((j-k)/j)>=0.00000000005)
	  p = 1;
      }
    } else {
      if (k<=0.0) {
	if (fabs((k-j)/j)>=0.00000000005)
	  p = -1;
      } else {
	if (fabs((k-j)/k)>=0.00000000005)
	  p = -1;
      }
    }
    if ((c[0] == '<' && p<0) || (c[0] == '>' && p>0) || (c[0] == '=' && !p)) {
      *n[m] = 1;
    }
  }
  if (m > -1)
    o = n[m];
  if (stack->data && m > -1) {
    addToListEnd(stack, n[m]);
  }
  m++;
  stackiter = NULL;
  if (stack->data)
    stackiter = stack;
  while (stackiter) {
    j = *(double*)stackiter->data;
    if (!(stackiter->next)) {
      o = (double*)stackiter->data;
      if (j) *o = 1; else *o = 0;
      break;
    }
    stackiter = stackiter->next;
    c = stackiter->data;
    stackiter = stackiter->next;
    o = (double*)stackiter->data;
    k = *o;
    if (((j && k) && c[0] == '&') || ((j || k) && c[0] == '|'))
      *o = 1;
    else
      *o = 0;
  }
  if (o == NULL)
    be->lasteval = 1;
  else
    be->lasteval = *o;
  for (p=0;p<m;p++) {
    free(n[p]);
  }
  free(n);
  freeList(stack);
  if (be->neg) be->lasteval = !be->lasteval;
  return be;
}

double evaluateValueAsBool(Value* v) {
  double i;
  Value* u;
  if (v->type == 'v') {
    u = valueFromName(((Variable*)v)->name);
    if (u == NULL) return NAN;
    return evaluateValueAsBool(u);
  }
  if (v->type == 'n') {
    return *((double*)v->data);
  }
  if (v->type == 'c') {
    u = evaluateFuncVal((FuncVal*)v);
    if (u == NULL) return NAN;
    i = evaluateValueAsBool(u);
    freeValue(u);
    return i;
  }
  if (v->type == 's') {
    return strlen(((String*)v->data)->val);
  }
  if (v->type == 'b') {
    return ((BoolExpr*)v)->lasteval;
  }
  if (v->type == 'l') {
    if (v->data)
      return lengthOfList(((List*)v->data)->next) +
	(double)((VarTree*)((List*)((List*)v->data)->data)->data)->count;
    else
      return 0.0;
  }
  if (v->type == 'd') {
    return evaluateValueAsBool(evaluateStatements((List*)v->data));
  }
  if (v->type == '0') {
    return 0;
  }
  return 1;
}

Value* valueFromName(char* name) {
  Value* v;
  List* vl;
  vl = varlist;
  do {
    v = findInTree(vl->data, name);
    vl = vl->next;
  } while (v == NULL && vl != NULL);
  if (v != NULL)
    return v;
  errmsgf("Variable named '%s' is not SET", name);
  return NULL;
}

double valueToDouble(Value* v) {
  double d, n;
  BoolExpr* be;
  Value* u;
  List* node;
  n = 0.0;
  if (v->type == 'n')
    return *(double*)v->data;
  if (v->type == 's')
    return atof(((String*)v->data)->val);
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
    if (!v->data)
      return 0.0;
    for (node = ((List*)v->data)->next;node != NULL;node = node->next) {
      u = evaluateValue(node->data);
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
  }
  return n;
}
