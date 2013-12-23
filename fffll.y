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
 char* eq, *gt, *lt, *and, *or;

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
				  if ($1 == constants+32) {
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
	| NUM			{
				  double* n;
				  n = malloc(sizeof(double));
				  *n = $1;
				  $$ = newValue('n', n);
				}
	| VAR			{
				  int f, g, h, i, j, k, l, m;
				  char* n, *o;
				  l = lengthOfList(varnames);
				  h = 0;
				  m = 0;
				  for (k=0;$1[k] != '\0';k++) {
				    if ($1[k] == '.' && !h) {
				      h = k;
				      m = 1;
				    }
				  }
				  j = 0;
				  o = NULL;
				  f = 1;
				  for (i=0;i<l;i++) {
				    n = dataInListAtPosition(varnames, i);
				    if (n == NULL)
				      continue;
				    g = strlen(n);
				    if (f && k == g) {
				      for (j=0;j<k;j++) {
					if ($1[j] != n[j]) break;
				      }
				      if (j == k) {
					free($1);
					$1 = n;
					f = 0;
				      }
				    }
				    if (m && h == g) {
				      for (j=0;j<h;j++) {
					if ($1[j] != n[j]) break;
				      }
				      if (j == h) {
					o = n;
					m = 0;
				      }
				    }
				  }
				  if (j != k) {
				    addToListBeginning(varnames, $1);
				  }
				  if (h) {
				    $$ = (Value*)newItem($1, o);
				  } else {
				    $$ = (Value*)newVariable($1);
				  }
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
  int strsize = 25;
  const char* str[] = { "=", "<", ">", "&", "|", "stdin", "stdout", "stderr",
                        "DEF", "SET", "IF", "WHILE", "WRITE", "READ", "OPEN",
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
  globalvars = newTree(constants+10, stdfiles[0]);
  globalvars = insertInTree(globalvars, constants+16, stdfiles[1]);
  globalvars = insertInTree(globalvars, constants+24, stdfiles[2]);
  addToListBeginning(varnames, constants+10);
  addToListBeginning(varnames, constants+16);
  addToListBeginning(varnames, constants+24);
  addToListBeginning(varlist, globalvars);
  addToListBeginning(funcnames, constants+32);
  addToListBeginning(funcnames, constants+36);
  addToListBeginning(funcnames, constants+40);
  addToListBeginning(funcnames, constants+44);
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
  newBuiltinFuncDef(constants+32, &defDef, 0);
  newBuiltinFuncDef(constants+36, &setDef, 0);
  newBuiltinFuncDef(constants+40, &ifDef, 0);
  newBuiltinFuncDef(constants+44, &whileDef, 0);
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
  freeTree(globalvars);
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
