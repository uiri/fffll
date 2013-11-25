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

 typedef struct value Value;
 struct value {
   int refcount;
   void *data;
   char type;
 };

 FILE** stdinp; FILE** stdoutp; FILE** stderrp;

 int freeValue(Value* val);
 Value *evaluateValue(Value* v);

 typedef struct funcval FuncVal;

 struct funcval {
   int refcount;
   char* name;
   char type;
   List* arglist;
 };

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
   int i, l;
   fv->refcount--;
   if (fv->refcount < 1) {
     free(fv->name);
     l = lengthOfList(fv->arglist);
     for (i=0;i<l;i++) {
       freeValue(dataInListAtPosition(fv->arglist, i));
     }
     freeList(fv->arglist);     
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
   int i, l;
   be->refcount--;
   if (be->refcount == 0) {
     l = lengthOfList(be->stack);
     for (i=0;i<l;i += 2) {
       freeValue(dataInListAtPosition(be->stack, i));
     }
     freeList(be->stack);
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
       freeList(val->data);
     }
     if (val->type == 'n') {
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
   printf("%d\n", vv->refcount);
   return vv;
 }

 int freeVarVal(VarVal* vv) {
   vv->refcount--;
   printf("%d\n", vv->refcount);
   if (vv->refcount < 1) {
     vv->val->refcount--;
     freeValue(vv->val);
     free(vv);
     return 1;
   }
   return 0;
 }

 List* varlist;
 DynArray* globalvars;

 typedef struct funcdef FuncDef;

 struct funcdef {
   char* name;
   Value* (*evaluate)(FuncDef*, List*);
   List *statements;
   List *arguments;
 };

 Value* evaluateFuncDef(FuncDef* fd, List* arglist);

 FuncDef* newFuncDef(char* name, List* sl, List* al) {
   FuncDef* fd;
   fd = malloc(sizeof(FuncDef));
   fd->name = name;
   fd->statements = sl;
   fd->arguments = al;
   fd->evaluate = &evaluateFuncDef;
   return fd;
 }

 int freeFuncDef(FuncDef* fd) {
   int i, l;
   l = lengthOfList(fd->statements);
   for (i=0;i<l;i++) {
     freeFuncVal(dataInListAtPosition(fd->statements, i));
   }
   l = lengthOfList(fd->arguments);
   for (i=0;i<l;i++) {
     freeValue(dataInListAtPosition(fd->arguments, i));
   }
   free(fd);
   return 0;
 }

 int funcnum;
 FuncDef **funcdeftable;

 int hashName(char* name, int l) {
   int i, s;
   s = 0;
   for(i=0;name[i] != 0;i++)
     s += name[i];
   return s%l;
 }

 FuncDef *getFunction(char *name) {
   FuncDef* fd;
   int i, j, l;
   i = hashName(name, funcnum);
   l = strlen(name);
   while (i++) {
     fd = funcdeftable[i];
     if (strlen(fd->name) == l) {
       for (j=0;j<l;j++) {
	 if (fd->name[j] != name[j])
	   break;
       }
       if (j == l)
	 break;
     }
   }
   return fd;
 }

 Value *evaluateStatements(List* sl) {
   Value* s;
   int i,l;
   l = lengthOfList(sl);
   for (i=0;i<l;i++) {
     s = evaluateValue((Value*)sl->data);
   }
   return s;
 }

 Value* evaluateFuncVal(FuncVal* fv) {
   return evaluateFuncDef(getFunction(fv->name), fv->arglist);
 }

 int evaluateValueAsBool(Value* v) {
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
     return evaluateValueAsBool(evaluateFuncVal((FuncVal*)v));
   }
   if (v->type == 'd') {
     return evaluateValueAsBool(evaluateStatements((List*)v->data));
   }
   /*if (v->type == 'v') {
     return evaluateValueAsBool(getValueFromVariable((Variable*)v);
     }*/
   return 1;
 }

 Value* evaluateBoolExpr(BoolExpr* be) {
   /*int i, j, k, l;
   char* c;
   Value* v;*/
   if (be->lasteval == -1)
     be->lasteval = 0;
   be->lasteval = !be->lasteval;
   /*l = lengthOfList(be->stack);
   i = 1;
   j = evaluateValueAsBool((Value*)dataInListAtPosition(be->stack, 0));
   be->lasteval = 0;
   if (j)
     be->lasteval = 1;
   while (i<l) {
     c = (char*)dataInListAtPosition(be->stack, i++);
     k = evaluateValueAsBool((Value*)dataInListAtPosition(be->stack, i++));
     if (c[0] == '<') {
       be->lasteval = 0;
       if (j < k)
	 be->lasteval = 1;
     }
     }*/
   return (Value*)be;
 }

 Value *evaluateValue(Value* v) {
   if (v->type == 'c')
     return evaluateFuncVal((FuncVal*)v);
   if (v->type == 'b')
     return evaluateBoolExpr((BoolExpr*)v);
   if (v->type == 'd')
     return evaluateStatements((List*)v->data);
   return v->data;
 }

 List* evaluateList(List* l) {
   List* r;
   r = newList();
   while (l != NULL) {
     if (((Value*)l->data)->type == 'b') {
       addToListEnd(r, l->data);
     } else {
       addToListEnd(r, evaluateValue((Value*)l->data));
     }
   }
   return r;
 }

 int scope(FuncDef* fd, List* arglist) {
   return 0;
 }

 int descope() {
   return 0;
 }

 Value* evaluateFuncDef(FuncDef* fd, List* arglist) {
   Value* val;
   scope(fd, arglist);
   arglist = evaluateList(arglist);
   val = evaluateStatements(fd->statements);
   descope();
   return val;
 }

 List* lastParseTree;
 List* NullList;
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
					  lastParseTree = $$;
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
				  $$ = NullList;
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
  char* constants;
  char* str[3] ={ "stdin", "stdout", "stderr" };
  int i, j, l;
  Value* v;
  VarVal* vv;
  if (argc != 2) {
    printf("This program takes exactly one argument. The file to interpret\n");
    return 1;
  }
  constants = calloc(32, 1);
  constants[0] = '=';
  constants[2] = '<';
  constants[4] = '>';
  constants[6] = '&';
  constants[8] = '|';
  eq = constants;
  lt = constants+2;
  gt = constants+4;
  and = constants+6;
  or = constants+8;
  l = 10;
  for (i=0;i<3;i++) {
    for (j=0;j<strlen(str[i]);j++) {
      constants[l] = str[i][j];
      l++;
    }
    l++;
  }
  stdinp = &stdin;
  stdoutp = &stdout;
  stderrp = &stderr;
  NullList = newList();
  *NullList = EmptyList;
  fp = fopen(argv[1], "r");
  yyin = fp;
  funcnum = 10;
  varlist = newList();
  globalvars = newArray(3, sizeof(VarVal));
  v = newValue('f', stdinp);
  vv = newVarVal(constants+10, v);
  appendToArray(globalvars, vv);
  v = newValue('f', stdoutp);
  vv = newVarVal(constants+16, v);
  appendToArray(globalvars, vv);
  v = newValue('f', stderrp);
  vv = newVarVal(constants+23, v);
  appendToArray(globalvars, vv);
  yyparse();
  vv = getElementInArray(globalvars, 2);
  funcnum *= 4;
  funcdeftable = calloc(funcnum, sizeof(FuncDef));
  for (i=0;i<funcnum;i++) {
    if (funcdeftable[i] != NULL) {
      freeFuncDef(funcdeftable[i]);
    }
  }
  free(funcdeftable);
  for (i=0;i<globalvars->last;i++) {
    vv = getElementInArray(globalvars, i);
    freeVarVal(vv);
  }
  freeArray(globalvars);
  freeList(varlist);
  free(NullList);
  free(constants);
  return 0;
}
