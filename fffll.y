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
#include <signal.h>
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

 Value* stdfiles[3];
 FILE** stdinp, **stdoutp, **stderrp;

 List* varlist;
 List* varnames;
 List* stringlist;
 VarTree* globalvars;

 int strsize = 26;
 int funcnum = 15;
 Value* funcdeftable[16];

 List* lastParseTree;
 char* eq, *gt, *lt, *and, *or, *sq, *ty;
 char* anonfunc = "anonymous function";

void siginfo(int sig) {
  switch (sig) {
  case SIGABRT:
    errmsg("\nSIGABRT: Aborting");
    break;
  case SIGFPE:
    errmsg("\nSIGFPE: Arithmetic error");
    break;
  case SIGILL:
    errmsg("\nSIGILL: Illegal instruction");
    break;
  case SIGINT:
    errmsg("\nSIGINT: Interrupted");
    break;
  case SIGTERM:
    errmsg("\nSIGTERM: Terminating");
    break;
  default:
    errmsg("\nSignal caught. Exiting now");
    break;
  }
  _Exit(1);
}

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

%token <index> IND
%token <num> NUM
%token <symbol> RGX
%token <symbol> STR
%token <symbol> VAR

%type <llist> statementlist
%type <fv> funcall
%type <val> funcdef;
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
funcall		: VAR arglist		{
					  Variable* v;
					  v = parseVariable($1);
					  $$ = newFuncVal((Value*)v, $2, v->name, lineno);
					}
		| funcdef arglist	{
					  $$ = newFuncVal($1, $2, anonfunc, lineno);
					}
		| funcall arglist	{
					  char* name;
					  int i;
					  name = malloc(strlen($1->name) + 3);
					  for (i=0;$1->name[i];i++)
					    name[i] = $1->name[i];
					  name[i++] = '(';
					  name[i++] = ')';
					  name[i] = '\0';
					  $$ = newFuncVal((Value*)$1, $2, name, lineno);
					}
		;
funcdef		: '[' list ']' '{' statementlist '}'	{
							  $$ = newValue('a', newFuncDef($2, $5, 0));
							}
		| '[' ']' '{' statementlist '}'		{
							  $$ = newValue('a', newFuncDef(NULL, $4, 0));
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
					  if ($1->next == NULL)
					    $1->next = newList();
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
					  if ($$->data == NULL) {
					    $$->data = newList();
					    ((List*)$$->data)->data = newTree(var->name, $5);
					  } else {
					    ((List*)$$->data)->data = insertInTree(((List*)$$->data)->data, var->name, $5);
					  }
					  addToListEnd($$->data, var);
					}
		| VAR ':' value		{
					  Variable* var;
					  $$ = newList();
					  var = parseVariable($1);
					  $$->data = newList();
					  ((List*)$$->data)->data = newTree(var->name, $3);
					  addToListEnd($$->data, var);
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
		| value '?' value {
				  $$ = newBoolExpr($1);
				  addToListEnd($$->stack, ty);
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
	| funcdef		{
				  $$ = $1;
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
  /* The value between str's [] should be the value of strsize
   * Don't forget to change it at the top of the file */
  const char* str[] = { "=", "<", ">", "&", "|", "~", "?", "_stdin", "_stdout", "_stderr",
                        "_set", "_if", "_for", "_write", "_read", "_open",
                        "_add", "_mul", "_rcp", "_len", "_tok", "_cat", "_head",
                        "_tail", "_push", "_die"};
  int lenconstants;
  int i, j, k, l;
  Value* v;
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
  ty = constants+12;
  stdinp = malloc(sizeof(stdin));
  *stdinp = stdin;
  stdoutp = malloc(sizeof(stdout));
  *stdoutp = stdout;
  stderrp = malloc(sizeof(stderr));
  *stderrp = stderr;
  fp = fopen(argv[1], "r");
  yyin = fp;
  falsevalue = newValue('0', NULL);
  varnames = newList();
  varlist = newList();
  stringlist = newList();
  stdfiles[0] = newValue('f', stdinp);
  stdfiles[1] = newValue('f', stdoutp);
  stdfiles[2] = newValue('f', stderrp);
  funcdeftable[0] = newValue('a', newBuiltinFuncDef(&setDef, 0));
  funcdeftable[1] = newValue('a', newBuiltinFuncDef(&ifDef, 0));
  funcdeftable[2] = newValue('a', newBuiltinFuncDef(&forDef, 0));
  funcdeftable[3] = newValue('a', newBuiltinFuncDef(&writeDef, 0));
  funcdeftable[4] = newValue('a', newBuiltinFuncDef(&readDef, 1));
  funcdeftable[5] = newValue('a', newBuiltinFuncDef(&openDef, 1));
  funcdeftable[6] = newValue('a', newBuiltinFuncDef(&addDef, 1));
  funcdeftable[7] = newValue('a', newBuiltinFuncDef(&mulDef, 1));
  funcdeftable[8] = newValue('a', newBuiltinFuncDef(&rcpDef, 1));
  funcdeftable[9] = newValue('a', newBuiltinFuncDef(&lenDef, 1));
  funcdeftable[10] = newValue('a', newBuiltinFuncDef(&tokDef, 1));
  funcdeftable[11] = newValue('a', newBuiltinFuncDef(&catDef, 1));
  funcdeftable[12] = newValue('a', newBuiltinFuncDef(&headDef, 1));
  funcdeftable[13] = newValue('a', newBuiltinFuncDef(&tailDef, 1));
  funcdeftable[14] = newValue('a', newBuiltinFuncDef(&pushDef, 0));
  funcdeftable[15] = newValue('a', newBuiltinFuncDef(&dieDef, 0));
  /* don't forget to fix funcdeftable's declaration */
  globalvars = newTree(constants+14, stdfiles[0]);
  globalvars = insertInTree(globalvars, constants+15, stdfiles[0]);
  globalvars = insertInTree(globalvars, constants+22, stdfiles[1]);
  globalvars = insertInTree(globalvars, constants+23, stdfiles[1]);
  globalvars = insertInTree(globalvars, constants+30, stdfiles[2]);
  globalvars = insertInTree(globalvars, constants+31, stdfiles[2]);
  globalvars = insertInTree(globalvars, constants+38, funcdeftable[0]); /* _set */
  globalvars = insertInTree(globalvars, constants+39, funcdeftable[0]); /* set */
  globalvars = insertInTree(globalvars, constants+44, funcdeftable[1]); /* _if */
  globalvars = insertInTree(globalvars, constants+45, funcdeftable[1]); /* if */
  globalvars = insertInTree(globalvars, constants+48, funcdeftable[2]); /* _for */
  globalvars = insertInTree(globalvars, constants+49, funcdeftable[2]); /* for */
  globalvars = insertInTree(globalvars, constants+54, funcdeftable[3]); /* _write */
  globalvars = insertInTree(globalvars, constants+55, funcdeftable[3]); /* write */
  globalvars = insertInTree(globalvars, constants+62, funcdeftable[4]); /* _read */
  globalvars = insertInTree(globalvars, constants+63, funcdeftable[4]); /* read */
  globalvars = insertInTree(globalvars, constants+68, funcdeftable[5]); /* _open */
  globalvars = insertInTree(globalvars, constants+69, funcdeftable[5]); /* open */
  globalvars = insertInTree(globalvars, constants+74, funcdeftable[6]); /* _add */
  globalvars = insertInTree(globalvars, constants+75, funcdeftable[6]); /* add */
  globalvars = insertInTree(globalvars, constants+80, funcdeftable[7]); /* _mul */
  globalvars = insertInTree(globalvars, constants+81, funcdeftable[7]); /* mul */
  globalvars = insertInTree(globalvars, constants+86, funcdeftable[8]); /* _rcp */
  globalvars = insertInTree(globalvars, constants+87, funcdeftable[8]); /* rcp */
  globalvars = insertInTree(globalvars, constants+92, funcdeftable[9]); /* _len */
  globalvars = insertInTree(globalvars, constants+93, funcdeftable[9]); /* len */
  globalvars = insertInTree(globalvars, constants+98, funcdeftable[10]); /* _tok */
  globalvars = insertInTree(globalvars, constants+99, funcdeftable[10]); /* tok */
  globalvars = insertInTree(globalvars, constants+104, funcdeftable[11]); /* _cat */
  globalvars = insertInTree(globalvars, constants+105, funcdeftable[11]); /* cat */
  globalvars = insertInTree(globalvars, constants+110, funcdeftable[12]); /* _head */
  globalvars = insertInTree(globalvars, constants+111, funcdeftable[12]); /* head */
  globalvars = insertInTree(globalvars, constants+116, funcdeftable[13]); /* _tail */
  globalvars = insertInTree(globalvars, constants+117, funcdeftable[13]); /* tail */
  globalvars = insertInTree(globalvars, constants+122, funcdeftable[14]); /* _push */
  globalvars = insertInTree(globalvars, constants+123, funcdeftable[14]); /* push */
  globalvars = insertInTree(globalvars, constants+128, funcdeftable[15]); /* _die */ /* +134 next */
  globalvars = insertInTree(globalvars, constants+129, funcdeftable[15]); /* die */ /* +135 next */
  addToListBeginning(varnames, constants+14);
  addToListBeginning(varnames, constants+15);
  addToListBeginning(varnames, constants+22);
  addToListBeginning(varnames, constants+23);
  addToListBeginning(varnames, constants+30);
  addToListBeginning(varnames, constants+31);
  addToListBeginning(varnames, constants+38);
  addToListBeginning(varnames, constants+39);
  addToListBeginning(varnames, constants+44);
  addToListBeginning(varnames, constants+45);
  addToListBeginning(varnames, constants+48);
  addToListBeginning(varnames, constants+49);
  addToListBeginning(varnames, constants+54);
  addToListBeginning(varnames, constants+55);
  addToListBeginning(varnames, constants+62);
  addToListBeginning(varnames, constants+63);
  addToListBeginning(varnames, constants+68);
  addToListBeginning(varnames, constants+69);
  addToListBeginning(varnames, constants+74);
  addToListBeginning(varnames, constants+75);
  addToListBeginning(varnames, constants+80);
  addToListBeginning(varnames, constants+81);
  addToListBeginning(varnames, constants+86);
  addToListBeginning(varnames, constants+87);
  addToListBeginning(varnames, constants+92);
  addToListBeginning(varnames, constants+93);
  addToListBeginning(varnames, constants+98);
  addToListBeginning(varnames, constants+99);
  addToListBeginning(varnames, constants+104);
  addToListBeginning(varnames, constants+105);
  addToListBeginning(varnames, constants+110);
  addToListBeginning(varnames, constants+111);
  addToListBeginning(varnames, constants+116);
  addToListBeginning(varnames, constants+117);
  addToListBeginning(varnames, constants+122);
  addToListBeginning(varnames, constants+123);
  addToListBeginning(varnames, constants+128);
  addToListBeginning(varnames, constants+129);
  addToListBeginning(varlist, globalvars);
  parencount = malloc(16*sizeof(int));
  parencount[parencountind] = 0;
  curl_global_init(CURL_GLOBAL_ALL);
  signal(SIGABRT, siginfo);
  signal(SIGFPE, siginfo);
  signal(SIGILL, siginfo);
  signal(SIGINT, siginfo);
  signal(SIGTERM, siginfo);
  if (yyin == NULL) {
    errmsg("File does not exist.");
    i = 1;
    v = NULL;
  } else {
    i = 0;
    yyparse();
    free(parencount);
    v = evaluateStatements(lastParseTree);
  }
  cleanupFffll(v);
  return i;
}
