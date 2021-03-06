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
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "exception.h"
#include "typecast.h"

extern Value* falsevalue;
extern List* varlist;
extern List* stringlist;
extern List* funcnames;
extern List* jmplist;
extern List* varnames;
extern char* diemsg;
extern char* errmsglist[];

/* Internal helper functions */

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

size_t writeHttpBuffer(void* contents, size_t size, size_t nmemb, void* userp) {
  HttpVal* hv;
  hv = (HttpVal*)userp;

  hv->buf = realloc(hv->buf, hv->bufsize + (size*nmemb) + 1);
  if (hv->buf == NULL)
    return 0;
  memcpy(&(hv->buf[hv->bufsize]), contents, (size*nmemb));
  hv->bufsize += (size*nmemb);
  hv->buf[hv->bufsize] = '\0';

  return (size*nmemb);
}

/* External Function Definitions */

Value* addDef(FuncDef* fd, List* arglist) {
  double d, *n;
  List* al, *node;
  al = evaluateList(arglist->next);
  if (al == NULL)
    return NULL;
  n = malloc(sizeof(double));
  *n = 0.0;
  for (node=al;node != NULL;node = node->next) {
    d = valueToDouble(node->data);
    *n += d;
  }
  freeValueList(al);
  if (isnan(*n)) {
    free(n);
    return NULL;
  }
  return newValue('n', n);
}

Value* catDef(FuncDef* fd, List* arglist) {
  char* s, *t;
  int i, j, l;
  String* str;
  List* al, *node;
  al = evaluateList(arglist->next);
  if (arglist == NULL) {
    return raiseErr(1);
  }
  l = 8;
  i = 0;
  s = malloc(8);
  for (node=al;node != NULL;node = node->next) {
    t = valueToString(node->data);
    if (t == NULL) {
      free(s);
      return NULL;
    }
    for (j=0;t[j] != '\0';j++) {
      if (i == l) {
	l *= 2;
	s = realloc(s, l);
      }
      s[i++] = t[j];
    }
    s[i] = '\0';
    free(t);
  }
  str = newString(s);
  str = addToStringList(str);
  return newValue('s', str);
}

Value* dieDef(FuncDef* fd, List* arglist) {
  Value* v;
  if (arglist) {
    v = evaluateValue(arglist->next->data);
    if (v) {
      diemsg = valueToString(v);
    }
    freeValue(v);
  } else {
    makeErrMsg(0);
  }
  return recoverErr();
}

Value* forDef(FuncDef* fd, List* arglist) {
  Value* v, *u;
  BoolExpr* be;
  int bool;
  List* sl, *l;
  double* d, e, i;
  if (!arglist) {
    return raiseErr(2);
  }
  v = falsevalue;
  v->refcount++;
  if (arglist->next->next == NULL)
    v->refcount++;
  sl = (List*)((Value*)arglist->next->data)->data;
  be = NULL;
  bool = 0;
  if (arglist->next->next) {
    bool = 1;
    sl = (List*)((Value*)arglist->next->next->data)->data;
  }
  if (arglist->data != NULL) {
    u = findInTree(((List*)arglist->data)->data, ((Variable*)((List*)arglist->data)->next->data)->name);
    while((u->type == 'v' || u->type == 'c') && u != NULL) {
      u = evaluateValue(u);
    }
    if (u != NULL && u->type == 'l') {
      if (!varlist->data)
	varlist->data = newTree(((Variable*)((List*)arglist->data)->next->data)->name, NULL);
      l = NULL;
      if ((List*)u->data)
	l = ((List*)u->data)->next;
      if (l && ((Value*)l->data)->type == 'r') {
	d = malloc(sizeof(double));
	*d = valueToDouble(((Range*)l->data)->start);
	e = valueToDouble(((Range*)l->data)->end);
	if (((Range*)l->data)->increment == falsevalue) {
	  i = 1.0;
	  if (e<*d)
	    i = -1.0;
	} else {
	  i = valueToDouble(((Range*)l->data)->increment);
	}
	((Range*)l->data)->computed = newValue('n', d);
	varlist->data = insertInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name, ((Range*)l->data)->computed);
	if (*d < e && 0 < i) {
	  for (;*d<e;*d+=i) {
	    v = evaluateStatements(sl);
	    if (bool && ((be = evaluateBoolExpr(arglist->next->data)) == NULL || !be->lasteval)) {
	      break;
	    }
	  }
	} else if (e < *d && i < 0) {
	  for (;e<*d;*d+=i) {
	    v = evaluateStatements(sl);
	    if (bool && ((be = evaluateBoolExpr(arglist->next->data)) == NULL || !be->lasteval)) {
	      break;
	    }
	  }
	}
	/* freeValue(findInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name)); */
	varlist->data = deleteInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name);
      } else {
	while (l) {
	  varlist->data = insertInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name, l->data);
	  v = evaluateStatements(sl);
	  if (bool && ((be = evaluateBoolExpr(arglist->next->data)) == NULL || !be->lasteval)) {
	    break;
	  }
	  l = l->next;
	}
      }
    }
    if (bool && be == NULL) return NULL;
    return v;
  }
  if (!bool) {
    while (1) {
      v = evaluateStatements(sl);
      if (v == NULL) return NULL;
    }
    return v;
  }
  for (be = evaluateBoolExpr(arglist->next->data);
       be != NULL && be->lasteval;
       be = evaluateBoolExpr(arglist->next->data)) {
    if (arglist->next->next) {
      v = evaluateStatements(sl);
      if (v == NULL) return NULL;
    }
  }
  if (be == NULL) return NULL;
  return v;
}

Value* headDef(FuncDef* fd, List* arglist) {
  Value* v;
  if (lengthOfList(arglist->next) != 1) {
    return raiseErr(3);
  }
  v = evaluateValue(arglist->next->data);
  if (v->type != 'l') {
    return raiseErr(15);
  }
  if (!v->data || !((List*)v->data)->next) {
    return raiseErr(26);
  }
  return evaluateValue(((List*)v->data)->next->data);
}

Value* ifDef(FuncDef* fd, List* arglist) {
  int l;
  BoolExpr* be;
  List* cond;
  l = lengthOfList(arglist->next);
  if (l < 2) {
    return raiseErr(4);
  }
  cond = arglist->next;
  while (1) {
    be = evaluateBoolExpr(cond->data);
    if (be == NULL) return NULL;
    if (be->lasteval) {
      return evaluateStatements((List*)((Value*)cond->next->data)->data);
    }
    cond = cond->next->next;
    if (cond == NULL || cond->next == NULL)
      break;
  }
  if (cond) {
    return evaluateStatements((List*)((Value*)cond->data)->data);
  }
  return falsevalue;
}

Value* lenDef(FuncDef* fd, List* arglist) {
  double* a;
  Value* v;
  List* al, *node;
  if (arglist == NULL) {
    return raiseErr(5);
  }
  al = evaluateList(arglist->next);
  if (al == NULL)
    return NULL;
  a = malloc(sizeof(double));
  *a = 0.0;
  for (node=al;node != NULL;node = node->next) {
    v = node->data;
    if (v->type == 's') {
      *a += (double)strlen(v->data);
    } else if (v->type == 'd') {
      *a += (double)lengthOfList(v->data);
    } else if (v->type == 'l') {
      if (v->data) {
	*a += (double)lengthOfList(((List*)v->data)->next);
      }
    } else if (v->type != '0') {
      freeValueList(al);
      free(a);
      return raiseErr(16);
    }
  }
  freeValueList(al);
  return newValue('n', a);
}

Value* mulDef(FuncDef* fd, List* arglist) {
  double d, *n;
  List* al, *node;
  al = evaluateList(arglist->next);
  if (al == NULL)
    return NULL;
  n = malloc(sizeof(double));
  *n = 1.0;
  for (node=al;node != NULL;node = node->next) {
    d = valueToDouble(node->data);
    *n *= d;
  }
  freeValueList(al);
  if (isnan(d)) {
    free(n);
    return NULL;
  }
  return newValue('n', n);
}

Value* openDef(FuncDef* fd, List* arglist) {
  FILE** fp;
  Value* v;
  char* s;
  if (arglist == NULL) {
    return raiseErr(6);
  }
  v = evaluateValue(arglist->next->data);
  if (v == NULL) return NULL;
  s = valueToString(v);
  if ((s[0] == 'h' || s[0] == 'H') && (s[1] == 't' || s[1] == 'T') &&
      (s[2] == 't' || s[2] == 'T') && (s[3] == 'p' || s[3] == 'P') &&
      (s[4] == ':' || ((s[4] == 's' || s[4] == 'S') && s[5] == ':'))) {
    return (Value*)newHttpVal(s);
  }
  fp = malloc(sizeof(FILE*));
  *fp = fopen(s, "a+");
  free(s);
  return newValue('f', fp);
}

Value* pushDef(FuncDef* fd, List* arglist) {
  Value* v, *u;
  int l;
  List* node;
  l = lengthOfList(arglist);
  if (l < 2) {
    return raiseErr(7);
  }
  v = evaluateValue(arglist->next->data);
  if (v->type != 'l') {
    freeValue(v);
    return raiseErr(17);
  }
  for (node=arglist->next->next;node != NULL;node = node->next) {
    u = evaluateValue(node->data);
    if (((Value*)node->data)->type == 'v')
      u->refcount++;
    if (!v->data) {
      v->data = newList();
      ((List*)v->data)->next = newList();
    }
    addToListEnd(((List*)v->data)->next, u);
  }
  return v;
}

Value* rcpDef(FuncDef* fd, List* arglist) {
  double b, c, d, m, *n;
  int i, e;
  if (lengthOfList(arglist->next) != 1) {
    return raiseErr(8);
  }
  d = valueToDouble(arglist->next->data);
  if (isnan(d))
    return NULL;
  n = malloc(sizeof(double));
  *n = frexp(d, &e);
  e -= 1;
  for (i=0;i<e;i++) {
    *n *= 0.5;
  }
  c = 0.0;
  do {
    m = c;
    c = *n;
    b = *n * (-1.0);
    *n = fma(d,b,2.0);
    *n *= (-1.0)*b;
    if (*n == m)
      break;
  } while (c != *n);
  return newValue('n', n);
}

Value* readDef(FuncDef* fd, List* arglist) {
  char c;
  char* s;
  FILE* fp;
  HttpVal* hv;
  double n, l;
  int i, j;
  Value* v;
  String* str;
  if (arglist == NULL) {
    return raiseErr(9);
  }
  v = evaluateValue(arglist->next->data);
  if (v == NULL) return NULL;
  if (v->type != 'f' && v->type != 'h') {
    if (((Value*)arglist->next->data)->type != 'v' &&
	((Value*)arglist->next->data)->type != 'c')
      freeValue(v);
    return raiseErr(18);
  }
  if (v->type == 'h') {
    hv = (HttpVal*)v;
    if (hv->buf == NULL) {
      curl_easy_setopt(hv->curl, CURLOPT_WRITEFUNCTION, writeHttpBuffer);
      curl_easy_setopt(hv->curl, CURLOPT_WRITEDATA, (void*)hv);
      i = curl_easy_perform(hv->curl);
      if (i) {
	return raiseErr(23);
      }
    }
    if (hv->pos == hv->bufsize) {
      return raiseErr(24);
    }
    for (i=0;hv->buf[hv->pos+i] != '\n' && hv->buf[hv->pos+1] != '\0';i++);
    s = malloc(i+1);
    for (j=0;j<i;j++) {
      s[j] = hv->buf[hv->pos++];
    }
    s[j] = '\0';
    hv->pos++;
  } else {
    fp = *(FILE**)v->data;
    if (feof(fp)) {
      return raiseErr(24);
    }
    n = 0;
    if (fp != stdin && fp != stdout && fp != stderr &&
	lengthOfList(arglist->next) > 1) {
      v = evaluateValue(arglist->next->next->data);
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
  }
  str = newString(s);
  str = addToStringList(str);
  if (((Value*)arglist->next->data)->type != 'v')
    freeValue(v);
  return newValue('s', str);
}

Value* saveDef(FuncDef* fd, List* arglist) {
  Value* v, *u;
  jmp_buf* savebuf;
  String* str;
  int saved;
  savebuf = malloc(sizeof(jmp_buf));
  addToListBeginning(jmplist, savebuf);
  v = NULL;
  if (!arglist) {
    return raiseErr(10);
  }
  if (!setjmp(*savebuf)) {
    v = evaluateStatements((List*)((Value*)arglist->next->data)->data);
    free(savebuf);
    jmplist = deleteFromListBeginning(jmplist);
  } else {
    saved = 0;
    u = findInTree(((List*)arglist->data)->data, ((Variable*)((List*)arglist->data)->next->data)->name);
    while((u->type == 'v' || u->type == 'c') && u != NULL) {
      u = evaluateValue(u);
    }
    if (u != NULL && u->type == 'd') {
      if (!varlist->data)
	varlist->data = newTree(((Variable*)((List*)arglist->data)->next->data)->name, NULL);
      str = newString(diemsg);
      str = addToStringList(str);
      if (diemsg == str->val)
	str->refcount++;
      else
	diemsg = str->val;
      v = newValue('s', str);
      varlist->data = insertInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name, v);
      evaluateStatements(u->data);
      u = findInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name);
      if (u->data != str) {
	saved = 1;
      } else {
	freeValue(v);
      }
      v = u;
      varlist->data = deleteInTree(varlist->data, ((Variable*)((List*)arglist->data)->next->data)->name);
    }
    free(savebuf);
    jmplist = deleteFromListBeginning(jmplist);
    if (!saved) {
      return recoverErr();
    }
  }
  return v;
}

Value* setDef(FuncDef* fd, List* arglist) {
  Value* v, *u, *w;
  int i, j, k;
  List* l, *m, *vl;
  char* s;
  double d, a, c;
  if (lengthOfList(arglist->next) < 2) {
    return raiseErr(11);
  }
  if (((Value*)arglist->next->data)->type != 'v') {
    return raiseErr(19);
  }
  v = evaluateValue(arglist->next->next->data);
  if (v == NULL) {
    return NULL;
  }
  if (((Value*)arglist->next->next->data)->type == 'v')
    v->refcount++;
  u = NULL;
  vl = varlist;
  do {
    if (vl->data)
      u = findInTree(vl->data, ((Variable*)arglist->next->data)->name);
    vl = vl->next;
  } while (u == NULL && vl != NULL);
  if (((Variable*)arglist->next->data)->name[0] == '_' && u) {
    return v;
  }
  if (((Variable*)arglist->next->data)->indextype[0] != '0') {
    s = NULL;
    l = NULL;
    for (k=0;((Variable*)arglist->next->data)->indextype[k] != '0';k++) {
      if (l && !s) {
	u = l->data;
      }
      if (!u || u->type != 'l') {
	return raiseErr(25);
      }
      s = NULL;
      l = ((List*)u->data)->next;
      if (((Variable*)arglist->next->data)->indextype[k] == 'v') {
	w = evaluateValue(((Variable*)arglist->next->data)->index[k]);
	if (w == NULL) return NULL;
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
      } else if (((Variable*)arglist->next->data)->indextype[k] == 'n') {
	j = *(int*)((Variable*)arglist->next->data)->index[k];
      } else {
	s = (char*)((Variable*)arglist->next->data)->index[k];
	j = 0;
      }
      if (s) {
	m = ((List*)u->data)->data;
	if (((Variable*)arglist->next->data)->indextype[k+1] == '0') {
	  u = findInTree(m->data, s);
	  if (u != NULL)
	    freeValue(u);
	  insertInTree(m->data, s, v);
	  return v;
	} else if (!m) {
	  u = findInTree(m->data, s);
	} else {
	  raiseErr(27);
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
	    a = valueToDouble(((Range*)l->data)->start);
	    c = valueToDouble(((Range*)l->data)->end);
	    if (((Range*)l->data)->increment == falsevalue) {
	      d = 1.0;
	      if (c<a)
		d = -1.0;
	    } else {
	      d = valueToDouble(((Range*)l->data)->increment);
	    }
	    i += (int)((c-a)/d);
	    if (i>=j) {
	      raiseErr(28);
	    }
	  }
	  l = l->next;
	}
	if (l == NULL) {
	  return raiseErr(26);
	}
      }
    }
    if (l->data != NULL)
      freeValue(l->data);
    l->data = v;
  } else {
    if (u != NULL && vl == varlist->next) {
      freeValue(u);
    }
    if (v->type == 'a' && ((FuncDef*)v->data)->arguments) {
      l = ((FuncDef*)v->data)->arguments->data;
      m = l;
      if (l != NULL) {
	while ( l->next != NULL) {
	  l = l->next;
	  s = ((Variable*)l->data)->name;
	  if ((u = findInTree(m->data, s)) && u->type == 'v' && ((Variable*)u)->
	      name == s) {
	    freeValue(u);
	    u = valueFromName(s);
	    m->data = insertInTree(m->data, s, u);
	  }
	}
      }
    }
    if (varlist->data)
      varlist->data = insertInTree(varlist->data,
                                   ((Variable*)arglist->next->data)->name,
                                   v);
    else
      varlist->data = newTree(((Variable*)arglist->next->data)->name, v);
  }
  return v;
}

Value* tailDef(FuncDef* fd, List* arglist) {
  Value* v;
  List* ll, *tl;
  if (lengthOfList(arglist->next) != 1) {
    return raiseErr(12);
  }
  ll = newList();
  v = evaluateValue(arglist->next->data);
  if (v->type != 'l') {
    return raiseErr(20);
  }
  ll->next = NULL;
  tl = NULL;
  if (v->data) {
    ll->next = cloneList(((List*)v->data)->next->next);
    tl = ((List*)v->data)->next->next;
  }
  while (tl != NULL) {
    ((Value*)tl->data)->refcount++;
    tl = tl->next;
  }
  return newValue('l', ll);
}

Value* tokDef(FuncDef* fd, List* arglist) {
  List* l, *al;
  Value* v;
  char* s, *t, *r;
  int h, i, j, k;
  String* str;
  if (lengthOfList(arglist->next) != 2) {
    return raiseErr(13);
  }
  al = evaluateList(arglist->next);
  if (al == NULL)
    return NULL;
  l = newList();
  l->next = newList();
  if (((Value*)al->data)->type != 's' ||
      ((Value*)al->next->data)->type != 's') {
    return raiseErr(21);
  }
  r = ((String*)((Value*)al->data)->data)->val;
  t = ((String*)((Value*)al->next->data)->data)->val;
  h = 0;
  for (i=0;r[i] != '\0';i++) {
    k = i;
    for (j=0;t[j] != '\0' && t[j] == r[i];j++) i++;
    if (t[j] != '\0' && r[i] != '\0') {
      i = k;
      continue;
    }
    k -= h;
    s = malloc(k+1);
    for (j=0;j<k;j++) {
      s[j] = r[h];
      h++;
    }
    s[j] = '\0';
    str = newString(s);
    str = addToStringList(str);
    v = newValue('s', str);
    addToListEnd(l->next, v);
    h = i;
  }
  freeValueList(al);
  return newValue('l', l);
}

Value* writeDef(FuncDef* fd, List* arglist) {
  char* s, *t;
  int i, j, l;
  FILE* fp;
  Value* v;
  HttpVal* hv;
  List* node;
  if (arglist == NULL) {
    return raiseErr(14);
  }
  v = evaluateValue(arglist->next->data);
  if (v == NULL) return NULL;
  if (v->type != 'f' && v->type != 'h') {
    return raiseErr(22);
  }
  j = -1;
  if (v->type == 'f') {
    fp = *(FILE**)v->data;
    l = 0;
    if (fp == stdout || fp == stderr) {
      l = 1;
    }
    for (node=arglist->next->next;node != NULL;node = node->next) {
      s = valueToString(node->data);
      if (s == NULL) {
	return NULL;
      }
      for (j=0;s[j] != '\0';j++) {
	fputc(s[j], fp);
      }
      free(s);
    }
    if (l || j == -1)
      fputc('\n', fp);
    fseek(fp, 0, SEEK_CUR);
  } else {
    l = 32;
    t = malloc(l);
    i = 0;
    for (node=arglist->next->next;node != NULL;node = node->next) {
      s = valueToString(node->data);
      for (j=0;s[j] != '\0';j++) {
	if (i == l) {
	  l *= 2;
	  t = realloc(t, l);
	}
	t[i++] = s[j];
      }
      free(s);
    }
    if (i == l) {
      l++;
      t = realloc(t, l);
    }
    t[i] = '\0';
    hv = (HttpVal*)v;
    curl_easy_setopt(hv->curl, CURLOPT_POSTFIELDSIZE, strlen(t));
    curl_easy_setopt(hv->curl, CURLOPT_POSTFIELDS, t);
    free(t);
  }
  return falsevalue;
}
