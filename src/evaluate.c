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

/* Internal helper functions */

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

Value* evaluateFuncDef(FuncDef* fd, List* arglist) {
  Value* val;
  if (scope(fd, arglist)) return NULL;
  val = evaluateStatements(fd->statements);
  if (val == NULL)
    return NULL;
  descope(val);
  return val;
}

List* evaluateList(List* l) {
  List* r;
  Value* v;
  FuncDef* fd;
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
      fd = NULL;
      if (((Value*)l->data)->type == 'c') {
	fd = evaluateValue(((FuncVal*)l->data)->val)->data;
      }
      if (((Value*)l->data)->type == 'v' || (fd && !fd->alloc)) {
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

Value* evaluateValue(Value* v) {
  Value* u, *w, *x;
  int i, j, k, r;
  List* l, *m;
  char* s;
  double* d, a, b, c;
  if (v->type == 'c') {
    return evaluateFuncVal((FuncVal*)v);
  }
  if (v->type == 'v') {
    l = NULL;
    s = NULL;
    u = NULL;
    x = v;
    a = NAN;
    b = NAN;
    for (k=0;((Variable*)v)->indextype[k] != '0';k++) {
      if (s == NULL) {
	if (l == NULL)
	  u = valueFromName(((Variable*)v)->name);
	else
	  u = l->data;
      }
      if (u == NULL) return NULL;
      if (u->type != 'l') {
        errmsg("Only lists can be indexed.");
        return NULL;
      }
      l = NULL;
      s = NULL;
      if (u->data)
	l = ((List*)u->data)->next;
      if (((Variable*)v)->indextype[k] == 'v') {
        w = evaluateValue(((Variable*)v)->index[k]);
	if (w->type == 's') {
	  s = ((String*)w->data)->val;
	  m = varnames;
	  j = strlen(s);
	  i = -1;
	  while (m != NULL) {
	    if (m->data && strlen(m->data) == j) {
	      for (i=0;i<j;i++)
		if (((char*)m->data)[i] != s[i])
		  break;
	    }
	    if (i == j) {
	      s = m->data;
	      break;
	    }
	    m = m->next;
	  }
	} else {
	  j = (int)valueToDouble(w);
	}
      } else if (((Variable*)v)->indextype[k] == 'n') {
        j = *(int*)((Variable*)v)->index[k];
      } else {
	s = (char*)((Variable*)v)->index[k];
	j = 0;
      }
      r = 0;
      if (s) {
	if (((List*)u->data)->data)
	  u = findInTree(((List*)((List*)u->data)->data)->data, s);
	else
	  u = NULL;
	if (u == NULL) {
	  errmsgf("Key '%s' not found", s);
	  return NULL;
	}
      } else {
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
	  if (((Value*)l->data)->type == 'r') {
	    r = i;
	    a = valueToDouble(((Range*)l->data)->start);
	    c = valueToDouble(((Range*)l->data)->end);
	    if (((Range*)l->data)->increment == falsevalue) {
	      b = 1.0;
	      if (c<a)
		b = -1.0;
	    } else {
	      b = valueToDouble(((Range*)l->data)->increment);
	    }
	    i += (int)((c-a)/b);
	    if (i>=j) {
	      break;
	    }
	  }
	  l = l->next;
	}
	if (l == NULL) {
	  errmsg("List index out of bounds");
	  return NULL;
	}
      }
    }
    if (s != NULL) {
      v = u;
    } else if (l != NULL) {
      v = l->data;
      if (v->type == 'r') {
	if (isnan(a)) {
	    a = valueToDouble(((Range*)l->data)->start);
	    c = valueToDouble(((Range*)l->data)->end);
	    if (((Range*)l->data)->increment == falsevalue) {
	      b = 1.0;
	      if (c<a)
		b = -1.0;
	    } else {
	      b = valueToDouble(((Range*)l->data)->increment);
	    }
	}
	d = malloc(sizeof(double));
	*d = a + (j-r)*b;
	if (((Range*)v)->computed != NULL)
	  freeValue(((Range*)v)->computed);
	((Range*)v)->computed = newValue('n', d);
	v = ((Range*)v)->computed;
      }
    } else {
      v = valueFromName(((Variable*)v)->name);
    }
    if (x == v) {
      errmsgf("Self referential variable '%s'. Exiting before stack overflow.", ((Variable*)x)->name);
      return NULL;
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
