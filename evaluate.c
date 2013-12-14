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

extern List* varlist;
extern VarTree* globalvars;

/* Internal helper functions */

int freeEachValueInTree(VarTree* vt, Value* v) {
  if (vt == NULL)
    return 1;
  if (vt->data != v)
    freeValue(vt->data);
  freeEachValueInTree(vt->left, v);
  freeEachValueInTree(vt->right, v);
  return 0;
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

int incEachRefcountInTree(VarTree* vt) {
  if (vt == NULL)
    return 1;
  ((Value*)vt->data)->refcount++;
  incEachRefcountInTree(vt->left);
  incEachRefcountInTree(vt->right);
  return 0;
}

int scope(FuncDef* fd, List* arglist) {
  VarTree* vt;
  List* fdname, *al;
  Value* v;
  fdname = fd->arguments;
  al = arglist;
  vt = copyTree(globalvars);
  incEachRefcountInTree(vt);
  while (fdname != NULL && al != NULL) {
    v = evaluateValue(al->data);
    v->refcount++;
    if (v == NULL) {
      addToListBeginning(varlist, vt);
      return 1;
    }
    vt = insertInTree(vt, ((Variable*)fdname->data)->name, v);
    if (((Value*)al->data)->type == 'c') {
      fd = getFunction(((FuncVal*)al->data)->name);
      if (fd->alloc == 1) {
	freeValue(v);
      }
    }
    fdname = fdname->next;
    al = al->next;
  }
  addToListBeginning(varlist, vt);
  return 0;
}

/* External evaluation functions */

BoolExpr* evaluateBoolExpr(BoolExpr* be) {
  int i, l, m, p;
  double j, k, **n, *o;
  char* c, *s, *t;
  List* stack;
  Value* v, *u;
  stack = newList();
  l = lengthOfList(be->stack);
  m = -1;
  n = malloc(((l+1)/2)*sizeof(double*));
  o = NULL;
  i = 0;
  while (i<l) {
    n[++m] = calloc(1, sizeof(double));
    s = NULL;
    t = NULL;
    u = dataInListAtPosition(be->stack, i);
    if (u->type != 'b') {
      v = evaluateValue(u);
      if (v == NULL) {
	freeList(stack);
	m++;
	for (i=0;i<m;i++) {
	  free(n[i]);
	}
	free(n);
	return NULL;
      }
      if (v->type == 's') {
	s = v->data;
      }
      if (u->type != 'v')
	freeValue(v);
    }
    j = evaluateValueAsBool((Value*)dataInListAtPosition(be->stack, i++));
    if (isnan(j)) {
      m++;
      for (i=0;i<m;i++) {
	free(n[i]);
      }
      free(n);
      freeList(stack);
      return NULL;
    }
    if (j) *n[m] = 1;
    if (i == l) {
      addToListEnd(stack, n[m]);
      break;
    }
    n[++m] = calloc(1, sizeof(double));
    c = (char*)dataInListAtPosition(be->stack, i++);
    u = dataInListAtPosition(be->stack, i);
    if (u->type != 'b') {
      v = evaluateValue(u);
      if (v == NULL) {
	freeList(stack);
	m++;
	for (i=0;i<m;i++) {
	  free(n[i]);
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
    k = evaluateValueAsBool((Value*)dataInListAtPosition(be->stack, i++));
    if (isnan(k)) {
      m++;
      for (i=0;i<m;i++) {
	free(n[i]);
      }
      free(n);
      freeList(stack);
      return NULL;
    }
    if (c[0] == '|' || c[0] == '&') {
      addToListEnd(stack, n[m-1]);
      addToListEnd(stack, c);
      addToListEnd(stack, n[m]);
      continue;
    }
    if (t != NULL && s != NULL) {
      if (s == t && c[0] == '=') {
	  *n[m] = 1;
      } else {
	for (p=0;s[p] != '\0' && s[p] == t[p];p++);
	if ((c[0] == '<' && s[p] < t[p]) || (c[0] == '>' && s[p] > t[p]))
	  *n[m] = 1;
      }
    } else {
      p = 0;
      if (j>k) {
	if (j<0.0) {
	  if (((j-k)/k)>=0.0000005)
	    p = 1;
	} else if (j>0.0) {
	  if (((j-k)/j)>=0.0000005)
	    p = 1;
	}
      } else if (j<k) {
	if (k<0.0) {
	  if (((k-j)/j)>=0.0000005)
	    p = -1;
	} else if (k>0.0) {
	  if (((k-j)/k)>=0.0000005)
	    p = -1;
	}
      }
      if ((c[0] == '<' && p<0) || (c[0] == '>' && p>0) || (c[0] == '=' && !p)) {
	*n[m] = 1;
      }
    }
  }
  if (m > -1)
    o = n[m];
  m++;
  l = lengthOfList(stack);
  i = 0;
  while (i<l) {
    j = *(double*)dataInListAtPosition(stack, i++);
    if (i == l) {
      o = (double*)dataInListAtPosition(stack, --i);
      if (j) *o = 1; else *o = 0;
      break;
    }
    c = dataInListAtPosition(stack, i++);
    o = (double*)dataInListAtPosition(stack, i);
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
  for (i=0;i<m;i++) {
    free(n[i]);
  }
  free(n);
  freeList(stack);
  if (be->neg) be->lasteval = !be->lasteval;
  return be;
}

Value* evaluateFuncDef(FuncDef* fd, List* arglist) {
  Value* val;
  if (scope(fd, arglist)) return NULL;
  val = evaluateStatements(fd->statements);
  if (val == NULL)
    return NULL;
  descope(val);
  return val;
}

Value* evaluateFuncVal(FuncVal* fv) {
  FuncDef* fd;
  fd = getFunction(fv->name);
  if (fd == NULL) return NULL;
  return (*fd->evaluate)(fd, fv->arglist);
}

List* evaluateList(List* l) {
  List* r;
  Value* v;
  r = newList();
  while (l != NULL) {
    if (((Value*)l->data)->type == 'd') {
      ((Value*)l->data)->refcount++;
      addToListEnd(r, l->data);
    } else {
      v = evaluateValue((Value*)l->data);
      if (v == NULL) {
	freeValueList(r);
	return NULL;
      }
      if (((Value*)l->data)->type == 'v' || ((Value*)l->data)->type == 'c') {
	v->refcount++;
      }
      addToListEnd(r, v);
    }
    l = l->next;
  }
  return r;
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
    return lengthOfList((List*)v->data);
  }
  if (v->type == 'd') {
    return evaluateValueAsBool(evaluateStatements((List*)v->data));
  }
  if (v->type == '0') {
    return 0;
  }
  return 1;
}

Value* evaluateValue(Value* v) {
  FuncVal* fv;
  if (v->type == 'c') {
    fv = (FuncVal*)v;
    v = evaluateFuncVal((FuncVal*)v);
    if (v == NULL) {
      errmsgfd("In %s at line %d", fv->name, fv->lineno);
    }
    return v;
  }
  if (v->type == 'v') {
    v = valueFromName(((Variable*)v)->name);
    if (v == NULL) return NULL;
    return v;
  }
  if (v->type == 'b') {
    v = (Value*)evaluateBoolExpr((BoolExpr*)v);
    if (v == NULL) return NULL;
  }
  v->refcount++;
  return v;
}
