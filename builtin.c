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

extern Value* falsevalue;
extern List* varlist;
extern List* stringlist;
extern List* funcnames;

/* Internal helper functions */

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
    l = lengthOfList(v->data);
    s = malloc(4);
    s[0] = '[';
    k = 0;
    for (i=0;i<l;i++) {
      if (k) s[k] = ' ';
      k++;
      u = evaluateValue(dataInListAtPosition(v->data, i));
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
  String* str;
  arglist = evaluateList(arglist);
  if (arglist == NULL) {
    errmsg("CAT requires at least one argument");
    return NULL;
  }
  l = lengthOfList(arglist);
  k = 0;
  s = NULL;
  for (i=0;i<l;i++) {
    t = valueToString(dataInListAtPosition(arglist, i));
    if (t == NULL) {
      free(s);
      return NULL;
    }
    j = strlen(t);
    k += j;
    s = realloc(s, k+1);
    strncat(s, t, j);
    s[k] = '\0';
    free(t);
  }
  str = newString(s);
  str = addToStringList(str);
  return newValue('s', str);
}

Value* defDef(FuncDef* fd, List* arglist) {
  char* name, *fn;
  int i, j, k, l;
  l = lengthOfList(arglist);
  if (l < 3) {
    errmsg("Not enough arguments for DEF");
    return NULL;
  }
  name = valueToString(arglist->data);
  for (i=0;name[i] != '\0';i++) {
    if (name[i] > 90) {
      name[i] -= 32;
    }
  }
  l = lengthOfList(funcnames);
  k = strlen(name);
  for (i=0;i<l;i++) {
    fn = dataInListAtPosition(funcnames, i);
    if (fn != NULL && strlen(fn) == k) {
      for (j=0;j<k;j++) {
	if (fn[j] != name[j]) break;
      }
      if (j == k) {
	free(name);
	name = fn;
	break;
      }
    }
  }
  insertFunction(newFuncDef(name, ((Value*)arglist->next->data)->data,
			    ((Value*)arglist->next->next->data)->data, 0));
  ((Value*)arglist->data)->refcount++;
  return (Value*)arglist->data;
}

Value* headDef(FuncDef* fd, List* arglist) {
  Value* v;
  if (lengthOfList(arglist) != 1) {
    errmsg("HEAD only takes one argument");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v->type != 'l') {
    errmsg("HEAD's argument must be a list");
    return NULL;
  }
  return evaluateValue(((List*)v->data)->data);
}

Value* ifDef(FuncDef* fd, List* arglist) {
  int l;
  BoolExpr* be;
  l = lengthOfList(arglist);
  if (l < 2) {
    errmsg("Not enough arguments for IF");
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
    errmsg("Not enough arguments for LEN");
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
      errmsg("LEN only takes variables, function calls, strings, lists or "
	     "blocks as arguments");
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
    errmsg("Not enough arguments for OPEN");
    return NULL;
  }
  v = evaluateValue(arglist->data);
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
  int i, l;
  l = lengthOfList(arglist);
  if (l < 2) {
    errmsg("PUSH needs at least two arguments");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v->type != 'l') {
    errmsg("PUSH's first argument must be a list");
    freeValue(v);
    return NULL;
  }
  for (i=1;i<l;i++) {
    u = evaluateValue(dataInListAtPosition(arglist, i));
    addToListEnd(v->data, u);
  }
  return v;
}

Value* rcpDef(FuncDef* fd, List* arglist) {
  double b, d, *n;
  int i, e;
  if (lengthOfList(arglist) != 1) {
    errmsg("RCP only takes 1 argument, the number whose reciprocal"
	   " is to be computed");
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
  HttpVal* hv;
  double n, l;
  int i, j;
  Value* v;
  String* str;
  if (arglist == NULL) {
    errmsg("Not enough arguments for READ");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v == NULL) return NULL;
  if (v->type != 'f' && v->type != 'h') {
    errmsg("READ only takes a file or http request as its argument");
    if (((Value*)arglist->data)->type != 'v' &&
	((Value*)arglist->data)->type != 'c')
      freeValue(v);
    return NULL;
  }
  if (v->type == 'h') {
    hv = (HttpVal*)v;
    if (hv->buf == NULL) {
      curl_easy_setopt(hv->curl, CURLOPT_WRITEFUNCTION, writeHttpBuffer);
      curl_easy_setopt(hv->curl, CURLOPT_WRITEDATA, (void*)hv);
      i = curl_easy_perform(hv->curl);
      if (i) {
	errmsgf("READ failed to read %s", hv->url);
      }
    }
    if (hv->pos == hv->bufsize) {
      errmsg("Attempt to READ past the end of a HTTP response");
      return NULL;
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
      errmsg("Attempt to READ past the end of a file");
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
  }
  str = newString(s);
  str = addToStringList(str);
  if (((Value*)arglist->data)->type != 'v')
    freeValue(v);
  return newValue('s', str);
}

Value* retDef(FuncDef* fd, List* arglist) {
  Value* v;
  if (arglist == NULL) {
    falsevalue->refcount++;
    return falsevalue;
  }
  if (((Value*)arglist->data)->type != 'd') {
    errmsg("RETURN only takes a statement block as its argument");
    return NULL;
  }
  v = evaluateStatements(((Value*)arglist->data)->data);
  v->refcount++;
  return v;
}

Value* setDef(FuncDef* fd, List* arglist) {
  Value* v, *u;
  int i, j, k;
  List* l, *m;
  if (lengthOfList(arglist) < 2) {
    errmsg("Not enough arguments for SET");
    return NULL;
  }
  if (((Value*)arglist->data)->type != 'v') {
    errmsg("SET requires a variable to be its first argument");
    return NULL;
  }
  v = evaluateValue(arglist->next->data);
  if (v == NULL) {
    return NULL;
  }
  u = findInTree(varlist->data, ((Variable*)arglist->data)->name);
  if (((Variable*)arglist->data)->indextype[0] == 'n' || ((Variable*)arglist->data)->indextype[0] == 'v') {
    l = NULL;
    for (k=0;((Variable*)arglist->data)->indextype[k] == 'n' || ((Variable*)arglist->data)->indextype[k] == 'v';k++) {
      if (l != NULL) {
	u = l->data;
      }
      if (u->type != 'l') {
	errmsg("Only lists can be indexed.");
	return NULL;
      }
      l = u->data;
      if (((Variable*)arglist->data)->indextype[k] == 'v') {
	u = evaluateValue(((Variable*)arglist->data)->index[k]);
	if (u == NULL) return NULL;
	j = (int)valueToDouble(u);
      } else {
	j = *(int*)((Variable*)arglist->data)->index[k];
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
    if (l->data != NULL)
      freeValue(l->data);
    l->data = v;
  } else {
    if (u != NULL) {
      freeValue(u);
    }
    varlist->data = insertInTree(varlist->data, ((Variable*)arglist->data)->name,
                                 v);
  }
  return v;
}

Value* tailDef(FuncDef* fd, List* arglist) {
  Value* v;
  List* ll, *tl;
  if (lengthOfList(arglist) != 1) {
    errmsg("TAIL only takes one argument");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v->type != 'l') {
    errmsg("TAIL's argument must be a list");
    return NULL;
  }
  ll = cloneList(((List*)v->data)->next);
  tl = ((List*)v->data)->next;
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
  int h, i, j, k;
  String* str;
  if (lengthOfList(arglist) != 2) {
    errmsg("TOK takes exactly two arguments, a string to tokenize and a "
	   "delimiter");
    return NULL;
  }
  arglist = evaluateList(arglist);
  if (arglist == NULL)
    return NULL;
  l = newList();
  if (((Value*)arglist->data)->type != 's' ||
      ((Value*)arglist->next->data)->type != 's') {
    errmsg("The arguments to TOK must be strings");
    return NULL;
  }
  r = ((String*)((Value*)arglist->data)->data)->val;
  t = ((String*)((Value*)arglist->next->data)->data)->val;
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
    addToListEnd(l, v);
    h = i;
  }
  freeValueList(arglist);
  return newValue('l', l);
}

Value* whileDef(FuncDef* fd, List* arglist) {
  Value* v;
  BoolExpr* be;
  if (lengthOfList(arglist) < 2) {
    errmsg("Not enough arguments for WHILE");
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
  char* s, *t;
  int h, i, j, k, l;
  FILE* fp;
  Value* v;
  HttpVal* hv;
  if (arglist == NULL) {
    errmsg("Not enough arguments for WRITE");
    return NULL;
  }
  v = evaluateValue(arglist->data);
  if (v == NULL) return NULL;
  if (v->type != 'f' && v->type != 'h') {
    errmsg("WRITE only takes a file or a http request as its first argument");
    return NULL;
  }
  l = lengthOfList(arglist);
  j = -1;
  if (v->type == 'f') {
    fp = *(FILE**)v->data;
    k = 0;
    if (fp == stdout || fp == stderr) {
      k = 1;
    }
    for (i=1;i<l;i++) {
      s = valueToString(dataInListAtPosition(arglist, i));
      if (s == NULL) {
	return NULL;
      }
      for (j=0;s[j] != '\0';j++) {
	fputc(s[j], fp);
      }
      free(s);
    }
    if (k || j == -1)
      fputc('\n', fp);
    fseek(fp, 0, SEEK_CUR);
  } else {
    k = 32;
    t = malloc(k);
    h = 0;
    for (i=1;i<l;i++) {
      s = valueToString(dataInListAtPosition(arglist, i));
      for (j=0;s[j] != '\0';j++) {
	if (h == k) {
	  k *= 2;
	  t = realloc(t, k);
	}
	t[h++] = s[j];
      }
      free(s);
    }
    if (h == k) {
      k++;
      t = realloc(t, k);
    }
    t[h] = '\0';
    hv = (HttpVal*)v;
    curl_easy_setopt(hv->curl, CURLOPT_POSTFIELDSIZE, strlen(t));
    curl_easy_setopt(hv->curl, CURLOPT_POSTFIELDS, t);
  }
  return falsevalue;
}
