%{
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
#include "list.h"
#include "array.h"

extern int yylex();
extern FILE *yyin;
extern int lineno;

#define YYERROR_VERBOSE

void yyerror(const char *msg) {
  printf("ERROR(PARSER): %s\n", msg);
}

 FILE** stdinp; FILE** stdoutp; FILE** stderrp;
 char* constants; int lenconstants;

 typedef struct value Value;
 struct value {
   int refcount;
   void *data;
   char type;
 };

 int freeValue(Value* val);
 Value *evaluateValue(Value* v);

 Value* falsevalue;

 typedef struct funcval FuncVal;

 struct funcval {
   int refcount;
   char* name;
   char type;
   List* arglist;
 };

 List* evaluateList(List* l) {
   List* r;
   r = newList();
   while (l != NULL) {
     if (((Value*)l->data)->type == 'b') {
       ((Value*)l->data)->refcount++;
       addToListEnd(r, l->data);
     } else {
       addToListEnd(r, evaluateValue((Value*)l->data));
     }
     l = l->next;
   }
   return r;
 }

 int freeValueList(List* r) {
   int i, l;
   l = lengthOfList(r);
   for (i=0;i<l;i++) {
     freeValue(dataInListAtPosition(r, i));
   }
   freeList(r);
   return 0;
 }

 FuncVal *newFuncVal(char* name, List* arglist) {
   FuncVal* fv;
   fv = malloc(sizeof(FuncVal));
   fv->refcount = 1;
   fv->name = name;
   fv->type = 'c';
   fv->arglist = arglist;
   return fv;
 }

 int freeFuncVal(FuncVal* fv) {
   fv->refcount--;
   if (fv->refcount < 1) {
     free(fv->name);
     freeValueList(fv->arglist);
     free(fv);
     return 1;
   }
   return 0;
 }

 typedef struct boolval BoolExpr;

 struct boolval {
   int refcount;
   List* stack;
   char type;
   int neg;
   int lasteval;
 };

 BoolExpr *newBoolExpr(Value* val) {
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

 int freeBoolExpr(BoolExpr* be) {
   be->refcount--;
   if (be->refcount < 1) {
     freeValueList(be->stack);
     free(be);
   }
   return 0;
 }

 typedef struct variable Variable;
 struct variable {
   int refcount;
   char* name;
   char type;
 };

 Variable *newVariable(char* name) {
   Variable *var;
   var = malloc(sizeof(Variable));
   var->refcount = 1;
   var->name = name;
   var->type = 'v';
   return var;
 }

 int freeVariable(Variable* var) {
   var->refcount--;
   if (var->refcount < 1) {
     free(var);
     return 1;
   }
   return 0;
 }

 Value *newValue(char type, void* data) {
   Value *val;
   val = malloc(sizeof(Value));
   val->refcount = 1;
   val->type = type;
   val->data = data;
   return val;
 }

 int freeValue(Value* val) {
   val->refcount--;
   if (val->refcount < 1) {
     if (val->type == 'l' || val->type == 'd') {
       freeValueList(val->data);
     }
     if (val->type == 'n' || val->type == 's') {
       free(val->data);
     }
     if (val->type == 'f') {
       if (val->data != stdinp && val->data != stdoutp && val->data != stderrp) {
	 fclose(*(FILE**)val->data);
       }
     }
     if (val->type == 'b') {
       freeBoolExpr((BoolExpr*)val);
       return 1;
     }
     if (val->type == 'c') {
       freeFuncVal((FuncVal*)val);
       return 1;
     }
     if (val->type == 'v') {
       freeVariable((Variable*)val);
       return 1;
     }
     free(val);
     return 1;
   }
   return 0;
 }

 List* varlist;
 List* varnames;
 DynArray* globalvars;
 
 typedef struct varval VarVal;

 struct varval {
   int refcount;
   char* name;
   char type;
   Value* val;
 };

 VarVal* newVarVal(char* name, Value* val) {
   VarVal* vv;
   vv = malloc(sizeof(VarVal));
   vv->refcount = 1;
   vv->name = name;
   vv->val = val;
   vv->type = 'w';
   val->refcount++;
   return vv;
 }

 int freeVarVal(VarVal* vv) {
   vv->refcount--;
   if (vv->refcount < 1) {
     freeValue(vv->val);
     free(vv);
     return 1;
   }
   return 0;
 }

 Value* appendToVarList(VarVal* vv) {
   appendToArray((DynArray*)varlist->data, vv);
   return vv->val;
 }

 VarVal* getVarValFromName(char* name) {
   int i, j, l;
   DynArray *va;
   VarVal* vv;
   va = varlist->data;
   l = strlen(name);
   i = 3;
   if (l == 5)
     i = 0;
   if (l == 6)
     i = 1;
   for(;i<va->last;i++) {
     vv = va->array[i];
     if (l == strlen(vv->name)) {
       for (j=0;j<l;j++) {
	 if (name[j] != vv->name[j])
	   break;
       }
       if (j == l)
	 break;
     }
     if (i == 0) i = 3;
   }
   if (i == va->last) return NULL;
   return va->array[i];
 }

 VarVal* varValFromName(char* name) {
   VarVal* vv;
   vv = getVarValFromName(name);
   if (vv == NULL) {
     printf("Variable named '%s' is not SET.\n", name);
     return NULL;
   }
   return vv;
 }

 typedef struct funcdef FuncDef;

 struct funcdef {
   char* name;
   Value* (*evaluate)(FuncDef*, List*);
   List *statements;
   List *arguments;
 };

 Value* evaluateFuncDef(FuncDef* fd, List* arglist);

 FuncDef* newFuncDef(char* name, List* al, List* sl) {
   FuncDef* fd;
   fd = malloc(sizeof(FuncDef));
   fd->name = name;
   fd->statements = sl;
   fd->arguments = al;
   fd->evaluate = &evaluateFuncDef;
   return fd;
 }

 int freeFuncDef(FuncDef* fd) {
   if (fd->name < constants || constants+lenconstants < fd->name) {
     free(fd->name);
   }
   free(fd);
   return 0;
 }

 int funcnum;
 FuncDef **funcdeftable;
 FuncDef *varAllocDefs[3];

 int hashName(char* name) {
   int i, s;
   s = 0;
   for(i=0;name[i] != '\0';i++)
     s += name[i];
   return s%funcnum;
 }

 int insertFunction(FuncDef* fd) {
   int i;
   i = hashName(fd->name);
   while (funcdeftable[i] != NULL) i++;
   funcdeftable[i] = fd;
   return 0;
 }

 FuncDef *getFunction(char *name) {
   FuncDef* fd;
   int i, j, l;
   i = hashName(name);
   l = strlen(name);
   for (j=0;j!=l;i++) {
     fd = funcdeftable[i];
     if (strlen(fd->name) == l) {
       for (j=0;j<l;j++) {
	 if (fd->name[j] != name[j])
	   break;
       }
     }
   }
   return fd;
 }

 int newBuiltinFuncDef(char* name, Value* (*evaluate)(FuncDef*, List*)) {
   FuncDef* fd;
   fd = newFuncDef(name, NULL, NULL);
   fd->evaluate = evaluate;
   return insertFunction(fd);
 }

 Value *evaluateStatements(List* sl) {
   Value* s, *t;
   int i,l;
   l = lengthOfList(sl);
   s = falsevalue;
   t = falsevalue;
   for (i=0;i<l;i++) {
     t = dataInListAtPosition(sl, i);
     t = evaluateValue(t);
     if (t == NULL) {
       return NULL;
     }
     if (t != falsevalue)
       s = t;
   }
   return s;
 }

 Value* evaluateFuncVal(FuncVal* fv) {
   FuncDef* fd;
   Value* v;
   fd = getFunction(fv->name);
   v = (*fd->evaluate)(fd, fv->arglist);
   /* This kind of sort of works as a solution to a reference counting bug. But not really :( */
   if (fd == varAllocDefs[0] || fd == varAllocDefs[1] || fd == varAllocDefs[2]) {
     v->refcount--;
   }
   return v;
 }

 double evaluateValueAsBool(Value* v) {
   VarVal* vv;
   int i;
   Value* u;
   if (v->type == '0') {
     return 0;
   }
   if (v->type == 'b') {
     return ((BoolExpr*)v)->lasteval;
   }
   if (v->type == 's') {
     return strlen((char*)v->data);
   }
   if (v->type == 'n') {
     return (int)*((double*)v->data);
   }
   if (v->type == 'l') {
     return lengthOfList((List*)v->data);
   }
   if (v->type == 'c') {
     u = evaluateFuncVal((FuncVal*)v);
     if (u == NULL) return INFINITY;
     i = evaluateValueAsBool(u);
     return i;
   }
   if (v->type == 'd') {
     return evaluateValueAsBool(evaluateStatements((List*)v->data));
   }
   if (v->type == 'v') {
     vv = varValFromName(((Variable*)v)->name);
     if (vv == NULL) return INFINITY;
     return evaluateValueAsBool(vv->val);
   }
   return 1;
 }

 BoolExpr* evaluateBoolExpr(BoolExpr* be) {
   int i, l, m;
   double j, k, **n, *o;
   char* c;
   List* stack;
   stack = newList();
   l = lengthOfList(be->stack);
   m = -1;
   n = malloc(((l+1)/2)*sizeof(double*));
   i = 0;
   while (i<l) {
     n[++m] = calloc(1, sizeof(double));
     j = evaluateValueAsBool((Value*)dataInListAtPosition(be->stack, i++));
     if (j == INFINITY) {
       m++;
       for (i=0;i<m;i++) {
	 free(n[i]);
       }
       free(n);
       freeList(stack);
       return NULL;
     }
     if (j) *n[m] = 1;
     c = (char*)dataInListAtPosition(be->stack, i++);
     if (c[0] == '|' || c[0] == '&') {
       addToListEnd(stack, n[m]);
       addToListEnd(stack, c);
       continue;
     }
     n[++m] = calloc(1, sizeof(double));
     k = evaluateValueAsBool((Value*)dataInListAtPosition(be->stack, i++));
     if (k == INFINITY) {
       m++;
       for (i=0;i<m;i++) {
	 free(n[i]);
       }
       free(n);
       freeList(stack);
       return NULL;
     }
     if ((c[0] == '<' && j < k) ||
	 (c[0] == '>' && j > k) ||
	 (c[0] == '=' && j == k)) {
       *n[m] = 1;
     }
   }
   o = n[m];
   m++;
   l = lengthOfList(stack);
   i = 0;
   while (i<l) {
     j = *(double*)dataInListAtPosition(stack, i++);
     if (i==l) {
       o = (double*)dataInListAtPosition(stack, i++);
       if (j) *o = 1; else *o = 0;
       break;
     }
     c = dataInListAtPosition(stack, i++);
     o = dataInListAtPosition(stack, i);
     k = *o;
     if (((j && k) && c[0] == '&') || ((j || k) && c[0] == '|'))
       *o = 1;
     else
       *o = 0;
   }
   be->lasteval = *o;
   for (i=0;i<m;i++) {
     free(n[i]);
   }
   free(n);
   freeList(stack);
   if (be->neg) be->lasteval = !be->lasteval;
   return be;
 }

 Value *evaluateValue(Value* v) {
   VarVal* vv;
   if (v->type == 'c') {
     v = evaluateFuncVal((FuncVal*)v);
     return v;
   } 
   if (v->type == 'b') {
     v = (Value*)evaluateBoolExpr((BoolExpr*)v);
   } else if (v->type == 'v') {
     vv = varValFromName(((Variable*)v)->name);
     if (vv == NULL) return NULL;
     v = vv->val;
   }
   v->refcount++;
   return v;
 }

 int scope(FuncDef* fd, List* arglist) {
   DynArray* da;
   List* fdname, *al;
   Value* v;
   fdname = fd->arguments;
   al = arglist;
   da = newArray(5, sizeof(VarVal));
   ((Value*)globalvars->array[0])->refcount++;
   ((Value*)globalvars->array[1])->refcount++;
   ((Value*)globalvars->array[2])->refcount++;
   appendToArray(da, globalvars->array[0]);
   appendToArray(da, globalvars->array[1]);
   appendToArray(da, globalvars->array[2]);
   while (fdname != NULL && al != NULL) {
     v = evaluateValue(al->data);
     if (v == NULL) {
       addToListBeginning(varlist, da);
       return 1;
     }
     appendToArray(da, newVarVal(((Variable*)fdname->data)->name, v));
     fdname = fdname->next;
     al = al->next;
   }
   addToListBeginning(varlist, da);
   return 0;
 }

 int descope() {
   DynArray* da;
   int i;
   da = varlist->data;
   for (i=0;i<da->last;i++) {
     freeVarVal(da->array[i]);
   }
   freeArray(da);
   varlist = deleteFromListBeginning(varlist);
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
     return atof((char*)v->data);
   if (v->type == 'b') {
     be = evaluateBoolExpr((BoolExpr*)v);
     if (be == NULL) return INFINITY;
     return (double)(be->lasteval);
   }
   if (v->type == 'c') {
     u = (Value*)evaluateFuncVal((FuncVal*)v);
     if (u == NULL) return INFINITY;
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
     if (u == NULL) return INFINITY;
     n = valueToDouble(u);
     freeValue(u);
   }
   return n;
 }

 char *valueToString(Value* v) {
   char* s, *t;
   int i, j, k, l, freet, m;
   VarVal* vv;
   BoolExpr* be;
   Value* u;
   freet = 0;
   l = 0;
   t = NULL;
   if (v->type == 'v') {
     vv = varValFromName(((Variable*)v)->name);
     if (vv == NULL) return NULL;
     s = valueToString(vv->val);
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

 Value* evaluateFuncDef(FuncDef* fd, List* arglist) {
   Value* val;
   if (scope(fd, arglist)) return NULL;
   val = evaluateStatements(fd->statements);
   val->refcount++;
   descope();
   return val;
 }

 Value* defDef(FuncDef* fd, List* arglist) {
   char* name;
   int i, l;
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
   insertFunction(newFuncDef(name, ((Value*)arglist->next->data)->data,
			     ((Value*)arglist->next->next->data)->data));
   ((Value*)arglist->data)->refcount++;
   return (Value*)arglist->data;
 }

 Value *setDef(FuncDef *fd, List* arglist) {
   VarVal* vv;
   Value* v, *u;
   if (lengthOfList(arglist) < 2) {
     printf("Not enough arguments for SET\n");
     return NULL;
   }
   u = arglist->next->data;
   v = evaluateValue(u);
   if (v == NULL) {
     return NULL;
   }
   vv = getVarValFromName(((Variable*)arglist->data)->name);
   if (vv != NULL) {
     freeValue(vv->val);
     vv->val = v;
   } else {
     vv = newVarVal(((Variable*)arglist->data)->name, v);
     v->refcount--;
     appendToVarList(vv);
   }
   return vv->val;
 }

 Value *ifDef(FuncDef *fd, List* arglist) {
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

 Value* whileDef(FuncDef *fd, List* arglist) {
   Value* v;
   BoolExpr *be;
   if (lengthOfList(arglist) < 2) {
     printf("Not enough arguments for WHILE\n");
     return NULL;
   }
   v = falsevalue;
   for (be = evaluateBoolExpr(arglist->data);
	be != NULL && be->lasteval;
	be = evaluateBoolExpr(arglist->data)) {
     v = evaluateStatements((List*)((Value*)arglist->next->data)->data);
     if (v == NULL) return NULL;
   }
   if (be == NULL) return NULL;
   return v;
 }

 Value* retDef(FuncDef *fd, List* arglist) {
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

 Value* writeDef(FuncDef *fd, List* arglist) {
   char* s, *t;
   int i, j, k, l, m, n, o;
   FILE** fp;
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
   fp = v->data;
   freeValue(v);
   k = 32;
   j = 0;
   s = calloc(k, 1);
   l = lengthOfList(arglist);
   if (fp == stdoutp || fp == stderrp) {
     o = 2;
   }
   for (i=1;i<l;i++) {
     t = valueToString(dataInListAtPosition(arglist, i));
     if (t == NULL) {
       free(s);
       return NULL;
     }
     n = strlen(t);
     if ((n+j+o) > k) {
       k *= 2;
       s = realloc(s, k);
     }
     for (m=0;m<n;m++) {
       s[j+m] = t[m];
     }
     j += n;
     free(t);
   }
   if (o == 2 || j == -1)
     s[j] = '\n';
   fputs(s, *fp);
   free(s);
   return falsevalue;
 }

 Value *readDef(FuncDef *fd, List* arglist) {
   char c;
   char* s;
   FILE* fp;
   int i, l;
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
   freeValue(v);
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
   v = newValue('s', s);
   return v;
 }
 
 Value *addDef(FuncDef *fd, List* arglist) {
   double *n, d;
   int i,l;
   Value* v;
   arglist = evaluateList(arglist);
   l = lengthOfList(arglist);
   n = malloc(sizeof(double));
   *n = 0.0;
   for (i=0;i<l;i++) {
     d = valueToDouble(dataInListAtPosition(arglist, i));
     if (d == INFINITY) {
       freeValueList(arglist);
       free(n);
       return NULL;
     }
     *n += d;
   }
   freeValueList(arglist);
   v = newValue('n', n);
   return v;
 }

 Value *mulDef(FuncDef *fd, List* arglist) {
   double *n, d;
   int i,l;
   Value* v;
   arglist = evaluateList(arglist);
   l = lengthOfList(arglist);
   n = malloc(sizeof(double));
   *n = 1.0;
   for (i=0;i<l;i++) {
     d = valueToDouble(dataInListAtPosition(arglist, i));
     if (d == INFINITY) {
       freeValueList(arglist);
       free(n);
       return NULL;
     }
     *n *= d;
   }
   freeValueList(arglist);
   v = newValue('n', n);
   return v;
 }

 List* lastParseTree;
 char* eq; char* gt; char* lt;
 char* and; char* or;

%}

%union {
  double num;
  char* symbol;
  struct List* llist;
  struct value* val;
  struct funcval* fv;
  struct boolval* be;
}

%token <num> NUM
%token <symbol> STR
%token <symbol> FUNC
%token <symbol> VAR

%type <llist> statementlist
%type <fv> funcall
%type <llist> arglist
%type <llist> list
%type <val> value;
%type <be> boolexpr;
%type <be> bexpr;
%type <be> compexpr;
%%

statementlist	: statementlist funcall	{
					  $$ = $1;
					  addToListEnd($$, $2);
					  if (lastParseTree != $$) {
					    lastParseTree = $$;
					  }
					}
		| funcall		{
					  $$ = newList();
					  addToListEnd($$, $1);
					  lastParseTree = $$;
	  				}
		;
funcall		: FUNC arglist	{ 
				  int i;
				  $$ = newFuncVal($1, $2);
				  i = strlen($1);
				  if (i == 3) {
				    if ($1[0] == 'D' && $1[1] == 'E' && $1[2] == 'F') {
				      funcnum++;
				    }
				  }
				}
		;
arglist		: '(' list ')'	{
				  $$ = $2;
				}
		| '(' ')'	{
				  $$ = NULL;
				}
		;
list		: list ',' value	{
					  addToListEnd($1, $3);
					  $$ = $1;
					}
		| value			{
					  $$ = newList();
					  addToListEnd($$, $1);
					}

boolexpr	: '!' '(' bexpr ')'	{
					  $$ = $3;
					  $$->neg = 1;
					}
		| '(' bexpr ')'		{
		  			  $$ = $2;
					}
		;
bexpr		: compexpr '&' compexpr
					{
					  $$ = $1;
					  addToListEnd($$->stack, and);
					  addListToList($$->stack, $3->stack);
					}
		| compexpr '|' compexpr
					{
					  $$ = $1;
					  addToListEnd($$->stack, or);
					  addListToList($$->stack, $3->stack);
					}
		| compexpr		{
					  $$ = $1;
					}
		;
compexpr	: value '>' value
				{
				  $$ = newBoolExpr($1);
				  addToListEnd($$->stack, gt);
				  addToListEnd($$->stack, $3);
				}
		| value '<' value
				{
				  $$ = newBoolExpr($1);
				  addToListEnd($$->stack, lt);
				  addToListEnd($$->stack, $3);
				}
		| value '=' value
				{
				  $$ = newBoolExpr($1);
				  addToListEnd($$->stack, eq);
				  addToListEnd($$->stack, $3);
				}
		| value		{
				  $$ = newBoolExpr($1);
				}
	;
value	: STR			{
				  $$ = newValue('s', $1);
				}
	| NUM			{
				  double* n;
				  n = malloc(sizeof(double));
				  *n = $1;
				  $$ = newValue('n', n);
				}
	| VAR			{
				  int i, j, k, l;
				  char* n, *vn;
				  l = lengthOfList(varnames);
				  k = strlen($1);
				  j = 0;
				  vn = $1;
				  for (i=0;i<l;i++) {
				    n = dataInListAtPosition(varnames, i);
				    if (n != NULL && k == strlen(n)) {
				      for (j=0;j<k;j++) {
					if ($1[k] != n[k]) break;
				      }
				      if (j != k) {
					free($1);
					$1 = n;
					break;
				      }
				    }
				  }
				  if ($1 == vn)
				    addToListBeginning(varnames, $1);
				  $$ = (Value*)newVariable($1);
				}
	| funcall		{
				  $$ = (Value*)$1;
				}
	| '[' list ']'		{
				  $$ = newValue('l', $2);
				}
	| '[' ']'		{
				  $$ = newValue('l', NULL);
				}
	| '{' statementlist '}'	{
				  $$ = newValue('d', $2);
				}
	| boolexpr		{
				  $$ = (Value*)$1;
				}
	;
%%

int main(int argc, char** argv) {
  FILE *fp;
  char* str[17] = { "=", "<", ">", "&", "|", "stdin", "stdout", "stderr", "DEF", "SET", "IF", "WHILE", "WRITE", "READ", "ADD", "MUL", "RETURN"};
  int i, j, k, l;
  Value* stdfiles[3];
  if (argc != 2) {
    printf("This program takes exactly one argument. The file to interpret\n");
    return 1;
  }
  lenconstants = sizeof(str);
  constants = calloc(lenconstants, 1);
  l = 0;
  for (i=0;i<17;i++) {
    k = strlen(str[i]);
    for (j=0;j<k;j++) {
      constants[l] = str[i][j];
      l++;
    }
    l++;
    if (k%2 == 0) {
      l++;
    }
  }
  eq = constants;
  lt = constants+2;
  gt = constants+4;
  and = constants+6;
  or = constants+8;
  stdinp = malloc(sizeof(stdin));
  *stdinp = stdin;
  stdoutp = malloc(sizeof(stdout));
  *stdoutp = stdout;
  stderrp = malloc(sizeof(stderr));
  *stderrp = stderr;
  fp = fopen(argv[1], "r");
  yyin = fp;
  funcnum = 10;
  falsevalue = newValue('0', NULL);
  varnames = newList();
  varlist = newList();
  globalvars = newArray(3, sizeof(VarVal));
  stdfiles[0] = newValue('f', stdinp);
  stdfiles[1] = newValue('f', stdoutp);
  stdfiles[2] = newValue('f', stderrp);
  appendToArray(globalvars, newVarVal(constants+10, stdfiles[0]));
  appendToArray(globalvars, newVarVal(constants+16, stdfiles[1]));
  appendToArray(globalvars, newVarVal(constants+24, stdfiles[2]));
  addToListBeginning(varlist, globalvars);
  yyparse();
  funcnum *= 4;
  funcdeftable = calloc(funcnum, sizeof(FuncDef));
  newBuiltinFuncDef(constants+32, &defDef);
  newBuiltinFuncDef(constants+36, &setDef);
  newBuiltinFuncDef(constants+40, &ifDef);
  newBuiltinFuncDef(constants+44, &whileDef);
  newBuiltinFuncDef(constants+50, &writeDef);
  newBuiltinFuncDef(constants+56, &readDef);
  newBuiltinFuncDef(constants+62, &addDef);
  newBuiltinFuncDef(constants+66, &mulDef);
  newBuiltinFuncDef(constants+70, &retDef);
  varAllocDefs[0] = getFunction("ADD");
  varAllocDefs[1] = getFunction("MUL");
  varAllocDefs[2] = getFunction("READ");
  evaluateStatements(lastParseTree);
  for (i=0;i<funcnum;i++) {
    if (funcdeftable[i] != NULL) {
      freeFuncDef(funcdeftable[i]);
    }
  }
  free(funcdeftable);
  /*freeValueList(lastParseTree);*/
  for (i=0;i<3;i++) {
    freeValue(stdfiles[i]);
  }
  for (i=0;i<globalvars->last;i++) {
    freeVarVal(globalvars->array[i]);
  }
  freeArray(globalvars);
  freeList(varlist);
  l = lengthOfList(varnames);
  for (i=0;i<l;i++) {
    free(dataInListAtPosition(varnames, i));
  }
  freeList(varnames);
  freeValue(falsevalue);
  free(stdinp);
  free(stdoutp);
  free(stderrp);
  free(constants);
  return 0;
}
