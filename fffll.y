%{
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

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "tree.h"
#include "value.h"

extern int yylex();
extern FILE* yyin;
extern int lineno;

int *parencount;
int parencountind = 0;
int parencountmax = 16;

#define YYERROR_VERBOSE

void yyerror(const char* msg) {
  printf("ERROR(PARSER): %s\n", msg);
}

 Value* falsevalue;
 char* constants;

 List* varlist;
 List* varnames;
 List* stringlist;
 VarTree* globalvars;

 int funcnum;
 FuncDef** funcdeftable;
 List* funcnames;

 List* lastParseTree;
 char* eq, *gt, *lt, *and, *or, *sq;

Variable* parseVariable(char* name) {
  int i, j, k, l;
  char* n;
  l = lengthOfList(varnames);
  for (k=0;name[k] != '\0';k++);
  j = 0;
  for (i=0;i<l;i++) {
    n = dataInListAtPosition(varnames, i);
    if (n == NULL)
      continue;
    if (k == strlen(n)) {
      for (j=0;j<k;j++) {
        if (name[j] != n[j]) break;
      }
      if (j == k) {
        free(name);
        name = n;
        break;
      }
    }
  }
  if (j != k) {
    addToListBeginning(varnames, name);
  }
  return newVariable(name);
}

%}

%union {
  int index;
  double num;
  char* symbol;
  struct List* llist;
  struct value* val;
  struct funcval* fv;
  struct boolval* be;
  struct VarTree* tree;
}

%token <symbol> FUNC
%token <index> IND
%token <num> NUM
%token <symbol> RGX
%token <symbol> STR
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
				  int i, j, k, l;
				  char* n;
				  l = lengthOfList(funcnames);
				  k = strlen($1);
				  j = 0;
				  for (i=0;i<l;i++) {
				    n = dataInListAtPosition(funcnames, i);
				    if (n != NULL && k == strlen(n)) {
				      for (j=0;j<k;j++) {
					if ($1[j] != n[j]) break;
				      }
				      if (j == k) {
					free($1);
					$1 = n;
					break;
				      }
				    }
				  }
				  if (j != k) {
				    addToListBeginning(funcnames, $1);
				  }
				  if ($1 == constants+34) {
				      funcnum++;
				  }
				  $$ = newFuncVal($1, $2, lineno);
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
					  addToListEnd($1->next, $3);
					  $$ = $1;
					}
		| NUM '.' IND '.' IND	{
					  Value* v;
					  double* n, d, e;
					  $$ = newList();
					  $$->next = newList();
					  e = $1;
					  d = ((double)$3) - e;
					  if ((e < $5 && d > 0) ||
					      (e > $5 && d < 0))
					    for (;e != $5;e += d) {
					      if ((e < $5 && e-d > $5) ||
					          (e > $5 && e-d < $5))
					        break;
					      n = malloc(sizeof(double));
					      *n = e;
					      v = newValue('n', n);
					      addToListEnd($$->next, v);
					    }
					}
		| NUM '.' IND	{
					  Value* v;
					  double* n, d, e;
					  $$ = newList();
					  $$->next = newList();
					  e = (double)$1;
					  d = 1;
					  if (e > $3)
					    d = -1;
					  for (;e != $3;e += d) {
					    n = malloc(sizeof(double));
					    *n = e;
					    v = newValue('n', n);
					    addToListEnd($$->next, v);
					  }
					}
		| value			{
					  $$ = newList();
					  $$->next = newList();
					  addToListEnd($$->next, $1);
					}
		| list ',' VAR ':' value {
					  Variable* var;
					  $$ = $1;
					  var = parseVariable($3);
					  if ($$->data == NULL)
					    $$->data = newTree(var->name, $5);
					  else
					    $$->data = insertInTree($$->data, var->name, $5);
					  freeVariable(var);
					}
		| VAR ':' value		{
					  Variable* var;
					  $$ = newList();
					  $$->next = newList();
					  var = parseVariable($1);
					  $$->data = newTree(var->name, $3);
					  freeVariable(var);
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
					  freeBoolExpr($3);
					}
		| compexpr '|' compexpr
					{
					  $$ = $1;
					  addToListEnd($$->stack, or);
					  addListToList($$->stack, $3->stack);
					  freeBoolExpr($3);
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
		| value '~' RGX
				{
				  Value* v;
				  String* s;
				  s = newString($3);
				  v = newValue('s', s);
				  $$ = newBoolExpr($1);
				  addToListEnd($$->stack, sq);
				  addToListEnd($$->stack, v);
				}
		| value		{
				  $$ = newBoolExpr($1);
				}
	;
value	: STR			{
				  String* s;
				  s = newString($1);
				  s = addToStringList(s);
				  $$ = newValue('s', s);
				}
	| value IND		{
				  int* n, i;
				  $$ = $1;
				  if ($$->type == 'v') {
				    n = malloc(sizeof(int));
				    *n = $2;
				    for (i=0;((Variable*)$$)->indextype[i] != '0';i++);
				    ((Variable*)$$)->indextype[i] = 'n';
				    ((Variable*)$$)->index[i] = n;
				    i++;
				    ((Variable*)$$)->indextype = realloc(((Variable*)$$)->indextype, ++i);
				    ((Variable*)$$)->index = realloc(((Variable*)$$)->index, i*sizeof(void*));
				    i--;
				    ((Variable*)$$)->indextype[i] = '0';
				    ((Variable*)$$)->index[i] = NULL;
				  }
				}
	| value '.' VAR		{
				  int i;
				  Variable* var;
				  $$ = $1;
				  if ($$->type == 'v') {
				    var = parseVariable($3);
				    for (i=0;((Variable*)$$)->indextype[i] != '0';i++);
				    ((Variable*)$$)->indextype[i] = 'u';
				    ((Variable*)$$)->index[i] = var->name;
				    i++;
				    ((Variable*)$$)->indextype = realloc(((Variable*)$$)->indextype, ++i);
				    ((Variable*)$$)->index = realloc(((Variable*)$$)->index, i*sizeof(void*));
				    i--;
				    ((Variable*)$$)->indextype[i] = '0';
				    ((Variable*)$$)->index[i] = NULL;
				    freeVariable(var);
				  }
				}
	| value '.' '[' value ']' {
				  int i;
				  $$ = $1;
				  if ($$->type == 'v') {
				    for (i=0;((Variable*)$$)->indextype[i] != '0';i++);
				    ((Variable*)$$)->indextype[i] = 'v';
				    ((Variable*)$$)->index[i] = $4;
				    i++;
				    ((Variable*)$$)->indextype = realloc(((Variable*)$$)->indextype, ++i);
				    ((Variable*)$$)->index = realloc(((Variable*)$$)->index, i*sizeof(void*));
				    i--;
				    ((Variable*)$$)->indextype[i] = '0';
				    ((Variable*)$$)->index[i] = NULL;
				  }
				}
	| NUM			{
				  double* n;
				  n = malloc(sizeof(double));
				  *n = $1;
				  $$ = newValue('n', n);
				}
	| VAR			{
				  $$ = (Value*)parseVariable($1);
				}
	| funcall               {
				  $$ = (Value*)$1;
				}
	| '[' list ']'          {
				  $$ = newValue('l', $2);
				}
	| '[' ']'               {
				  $$ = newValue('l', NULL);
				}
	| '{' statementlist '}' {
				  $$ = newValue('d', $2);
				}
	| boolexpr              {
				  $$ = (Value*)$1;
				}
	;
%%

int main(int argc, char** argv) {
  FILE* fp;
  FILE** stdinp, **stdoutp, **stderrp;
  /* The value between str's [] should be the value of strsize */
  int strsize = 26;
  const char* str[] = { "=", "<", ">", "&", "|", "~", "stdin", "stdout", "stderr",
                        "DEF", "SET", "IF", "FOR", "WRITE", "READ", "OPEN",
                        "ADD", "MUL", "RCP", "RETURN", "LEN", "TOK", "CAT",
                        "HEAD", "TAIL", "PUSH"};
  int lenconstants;
  int i, j, k, l;
  Value* stdfiles[3], *v;
  if (argc != 2) {
    printf("This program takes exactly one argument. The file to interpret\n");
    return 1;
  }
  lenconstants = sizeof(str);
  constants = calloc(lenconstants, 1);
  l = 0;
  for (i=0;i<strsize;i++) {
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
  sq = constants+10;
  stdinp = malloc(sizeof(stdin));
  *stdinp = stdin;
  stdoutp = malloc(sizeof(stdout));
  *stdoutp = stdout;
  stderrp = malloc(sizeof(stderr));
  *stderrp = stderr;
  fp = fopen(argv[1], "r");
  yyin = fp;
  funcnum = strsize - 8;
  falsevalue = newValue('0', NULL);
  funcnames = newList();
  varnames = newList();
  varlist = newList();
  stringlist = newList();
  stdfiles[0] = newValue('f', stdinp);
  stdfiles[1] = newValue('f', stdoutp);
  stdfiles[2] = newValue('f', stderrp);
  globalvars = newTree(constants+12, stdfiles[0]);
  globalvars = insertInTree(globalvars, constants+18, stdfiles[1]);
  globalvars = insertInTree(globalvars, constants+26, stdfiles[2]);
  addToListBeginning(varnames, constants+12);
  addToListBeginning(varnames, constants+18);
  addToListBeginning(varnames, constants+26);
  addToListBeginning(varlist, globalvars);
  addToListBeginning(funcnames, constants+34);
  addToListBeginning(funcnames, constants+38);
  addToListBeginning(funcnames, constants+42);
  addToListBeginning(funcnames, constants+46);
  addToListBeginning(funcnames, constants+50);
  addToListBeginning(funcnames, constants+56);
  addToListBeginning(funcnames, constants+62);
  addToListBeginning(funcnames, constants+68);
  addToListBeginning(funcnames, constants+72);
  addToListBeginning(funcnames, constants+76);
  addToListBeginning(funcnames, constants+80);
  addToListBeginning(funcnames, constants+88);
  addToListBeginning(funcnames, constants+92);
  addToListBeginning(funcnames, constants+96);
  addToListBeginning(funcnames, constants+100);
  addToListBeginning(funcnames, constants+106);
  addToListBeginning(funcnames, constants+112);
  parencount = malloc(16*sizeof(int));
  parencount[parencountind] = 0;
  curl_global_init(CURL_GLOBAL_ALL);
  yyparse();
  free(parencount);
  funcnum *= 4;
  i = 64;
  while (i<funcnum)
    i *= 2;
  funcnum = i - 1;
  funcdeftable = calloc(funcnum, sizeof(FuncDef));
  newBuiltinFuncDef(constants+34, &defDef, 0);
  newBuiltinFuncDef(constants+38, &setDef, 0);
  newBuiltinFuncDef(constants+42, &ifDef, 0);
  newBuiltinFuncDef(constants+46, &forDef, 0);
  newBuiltinFuncDef(constants+50, &writeDef, 0);
  newBuiltinFuncDef(constants+56, &readDef, 1);
  newBuiltinFuncDef(constants+62, &openDef, 1);
  newBuiltinFuncDef(constants+68, &addDef, 1);
  newBuiltinFuncDef(constants+72, &mulDef, 1);
  newBuiltinFuncDef(constants+76, &rcpDef, 1);
  newBuiltinFuncDef(constants+80, &retDef, 0);
  newBuiltinFuncDef(constants+88, &lenDef, 1);
  newBuiltinFuncDef(constants+92, &tokDef, 1);
  newBuiltinFuncDef(constants+96, &catDef, 1);
  newBuiltinFuncDef(constants+100, &headDef, 1);
  newBuiltinFuncDef(constants+106, &tailDef, 1);
  newBuiltinFuncDef(constants+112, &pushDef, 0);
  v = evaluateStatements(lastParseTree);
  for (i=0;i<funcnum;i++) {
    if (funcdeftable[i] != NULL) {
      free(funcdeftable[i]);
    }
  }
  free(funcdeftable);
  /*freeValueList(lastParseTree);*/
  for (i=0;i<3;i++) {
    freeValue(stdfiles[i]);
  }
  if (v != NULL && v != falsevalue) {
    freeValue(v);
  }
  /*freeTree(globalvars);*/
  freeList(varlist);
  l = lengthOfList(varnames) - 4;
  for (i=0;i<l;i++) {
    free(dataInListAtPosition(varnames, i));
  }
  freeList(varnames);
  l = lengthOfList(funcnames) + 7 - strsize;
  for (i=0;i<l;i++) {
    free(dataInListAtPosition(funcnames, i));
  }
  freeList(funcnames);
  freeValue(falsevalue);
  free(stdinp);
  free(stdoutp);
  free(stderrp);
  free(constants);
  curl_global_cleanup();
  return 0;
}
