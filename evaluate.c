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

/* Internal helper functions */

Value* valueFromName(char* name) {
  Value* v;
  List* vl;
  vl = varlist;
  do {
    v = findInTree(vl->data, name);
    vl = vl->next;
  } while (v == NULL && vl != NULL);
  if (v == NULL) {
    errmsgf("Variable named '%s' is not SET", name);
    return NULL;
  }
  return v;
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

/* External evaluation functions */

BoolExpr* evaluateBoolExpr(BoolExpr* be) {
  int i, l, m, p, erroff, ovec[PCREOVECCOUNT];
  double j, k, **n, *o;
  char* c, *s, *t;
  const char* err;
  List* stack, *prop;
  Value* v, *u, *w;
  pcre *re;
  re = NULL;
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
    w = NULL;
    u = dataInListAtPosition(be->stack, i);
    if (u->type != 'b') {
      v = evaluateValue(u);
      w = v;
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
	if (fabs((j-k)/k)>=0.0000005)
	  p = 1;
      } else {
	if (fabs((j-k)/j)>=0.0000005)
	  p = 1;
      }
    } else if (j<k) {
      if (k<=0.0) {
	if (fabs((k-j)/j)>=0.0000005)
	  p = -1;
      } else {
	if (fabs((k-j)/k)>=0.0000005)
	  p = -1;
      }
    }
    if ((c[0] == '<' && p<0) || (c[0] == '>' && p>0) || (c[0] == '=' && !p)) {
      *n[m] = 1;
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
  Value* v, *u;
  v = fv->val;
  u = NULL;
  fd = NULL;
  if (v->type != 'a') {
    u = v;
    v = evaluateValue(u);
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
    return lengthOfList(((List*)v->data)->next) + 
      (double)((VarTree*)((List*)((List*)v->data)->data)->data)->count;
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
  Value* u;
  int i, j, k;
  List* l, *m;
  if (v->type == 'c') {
    v = evaluateFuncVal((FuncVal*)v);
    return v;
  }
  if (v->type == 'v') {
    if (v == NULL) return NULL;
    l = NULL;
    for (k=0;((Variable*)v)->indextype[k] == 'n' || ((Variable*)v)->indextype[k] == 'v';k++) {
      if (l == NULL)
	u = valueFromName(((Variable*)v)->name);
      else
	u = l->data;
      if (u == NULL) return NULL;
      if (u->type != 'l') {
        errmsg("Only lists can be indexed.");
        return NULL;
      }
      l = ((List*)u->data)->next;
      if (((Variable*)v)->indextype[k] == 'v') {
        u = evaluateValue(((Variable*)v)->index[k]);
        j = (int)valueToDouble(u);
      } else {
        j = *(int*)((Variable*)v)->index[k];
      }
      m = l;
      if (j < 0) {
	for (;j<0;j++) {
	  m = m->next;
	}
	while (m != NULL) {
	  m = m->next;
	  l = l->next;
	}
      }
      for (i=0;i<j;i++) {
        if (l == NULL)
          break;
        l = l->next;
      }
      if (l == NULL) {
        errmsg("List index out of bounds");
        return NULL;
      }
    }
    if (l != NULL) {
      v = l->data;
    } else {
      v = valueFromName(((Variable*)v)->name);
    }
    return v;
  }
  if (v->type == 'b') {
    v = (Value*)evaluateBoolExpr((BoolExpr*)v);
    if (v == NULL) return NULL;
  }
  v->refcount++;
  return v;
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

double valueToDouble(Value* v) {
  double d, n;
  int i, l;
  BoolExpr* be;
  Value* u;
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
    l = lengthOfList(((List*)v->data)->next);
    for (i=0;i<l;i++) {
      u = evaluateValue(dataInListAtPosition(((List*)v->data)->next, i));
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
  if (v->type == 'l') {
    l = lengthOfList(((List*)v->data)->next);
    s = malloc(4);
    s[0] = '[';
    k = 0;
    for (i=0;i<l;i++) {
      if (k) s[k] = ' ';
      k++;
      u = evaluateValue(dataInListAtPosition(((List*)v->data)->next, i));
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
