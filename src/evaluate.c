#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "typecast.h"

extern Value* falsevalue;
extern List* varnames;

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
