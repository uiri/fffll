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
#include "compiler.h"
#include "tree.h"

extern int yylex();
extern FILE* yyin;
extern int lineno;

int *parencount;
int parencountind = 0;
int parencountmax = 16;

#define YYERROR_VERBOSE

void yyerror(const char* msg) {
  fprintf(stderr, "%s, try line %d\n", msg, lineno);
}

 Value* falsevalue;
 char* constants = "=\0<\0>\0&\0|\0~\0?\0_stdin\0\0_stdout\0_stderr\0_set\0\0_if\0"
	"_for\0\0_write\0\0_read\0_open\0_add\0\0_mul\0\0_rcp\0\0_len\0\0_tok\0\0_cat\0\0_head"
	"\0_tail\0_push\0_die\0\0_save\0";

 Value* stdfiles[3];
 FILE** stdinp, **stdoutp, **stderrp;

 List* varlist;
 List* varnames;
 List* stringlist;
 List* numlist;
 List* constlist;
 List* jmplist;
 List* localvarlist;
 VarTree* globalvars;

 short curl_init = 0;
 short repl = 0;

 int lenconstants;
 int strsize = 27;
 int funcnum = 16;
 Value* funcdeftable[17];

 List* yyinlist;
 List* parseTreeList;
 char* eq, *gt, *lt, *and, *or, *sq, *ty;
 char* anonfunc = "anonymous function";
 char* argfilename;

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

int addTreeKeysToList(List* l, VarTree* vt) {
  if (vt == NULL)
    return 0;
  addToListEnd(l, vt->key);
  addTreeKeysToList(l, vt->left);
  addTreeKeysToList(l, vt->right);
  return 0;
}

%}

%union {
  int index;
  double num;
  char* symbol;
  struct List* llist;
  struct rangelist* rrange;
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
%token <symbol> EOFSYM

%type <llist> statementlist;
%type <fv> funcall;
%type <val> funcdef;
%type <llist> arglist;
%type <llist> list;
%type <rrange> range;
%type <val> value;
%type <be> boolexpr;
%type <be> bexpr;
%type <be> compexpr;
%%

statementlist	: statementlist EOFSYM	{
					  List* results;
					  Value* v;
					  addToListBeginning(varlist, NULL);
					  v = evaluateStatements(parseTreeList->data);
					  if (v != NULL && v != falsevalue) {
					    freeValue(v);
					  }
					  results = newList();
					  results->data = newList();
					  ((List*)results->data)->data = varlist->data;
					  addTreeKeysToList(results->data, varlist->data);
					  varlist->next->data = insertInTree(varlist->next->data, $2, newValue('l', results));
					  varlist = deleteFromListBeginning(varlist);
					  parseTreeList = deleteFromListBeginning(parseTreeList);
					}
		| statementlist funcall	{
					  $$ = parseTreeList->data;
					  if (repl) {
					    $$->data = $2;
					  } else {
					    addToListEnd($$, $2);
					  }
					}
		| funcall		{
					  $$ = newList();
					  addToListEnd($$, $1);
					  addToListBeginning(parseTreeList, $$);
					}
		;
funcall		: VAR arglist		{
					  Variable* v;
					  Value* val;
					  char* s;
					  v = parseVariable($1);
					  /* test if curl not initialized and open being used */
					  if (!curl_init && (v->name == constants+69 || v->name == constants+68)) {
					    curl_init = 1;
					    curl_global_init(CURL_GLOBAL_ALL);
					  }
					  $$ = newFuncVal((Value*)v, $2, v->name, lineno);
					  if (repl && !parencount[parencountind]) {
					    val = evaluateFuncVal($$);
					    freeValue((Value*)$$);
					    s = valueToString(val);
					    printf("=> %s\n", s);
					    free(s);
					    if (val == NULL)
					      YYABORT;
					    $$ = (FuncVal*)val;
					  }
					}
		| value '.' VAR arglist	{
					  Variable* v, *var;
					  int i;
					  Value* val;
					  char* s;
					  v = (Variable*)$1;
					  if ($1->type == 'v') {
					    var = parseVariable($3);
					    v = (Variable*)$1;
					    for (i=0;v->indextype[i] != '0';i++);
					    v->indextype[i] = 'u';
					    v->index[i] = var->name;
					    i++;
					    v->indextype = realloc(((Variable*)$$)->indextype, ++i);
					    v->index = realloc(((Variable*)$$)->index, i*sizeof(void*));
					    i--;
					    v->indextype[i] = '0';
					    v->index[i] = NULL;
					    freeVariable(var);
					  }
					  $$ = newFuncVal((Value*)v, $4, v->name, lineno);
					  if (repl && !parencount[parencountind]) {
					    val = evaluateFuncVal($$);
					    freeValue((Value*)$$);
					    s = valueToString(val);
					    printf("=> %s\n", s);
					    free(s);
					    if (val == NULL)
					      YYABORT;
					    $$ = (FuncVal*)val;
					    printf(">>> ");
					  }
					}
		| funcdef arglist	{
					  Value* val;
					  char* s;
					  $$ = newFuncVal($1, $2, anonfunc, lineno);
					  if (repl && !parencount[parencountind]) {
					    val = evaluateFuncVal($$);
					    freeValue((Value*)$$);
					    s = valueToString(val);
					    printf("=> %s\n", s);
					    free(s);
					    if (val == NULL)
					      YYABORT;
					    $$ = (FuncVal*)val;
					    printf(">>> ");
					  }
					}
		| funcall arglist	{
					  char* name;
					  int i;
					  Value* val;
					  char* s;
					  name = malloc(strlen($1->name) + 3);
					  for (i=0;$1->name[i];i++)
					    name[i] = $1->name[i];
					  name[i++] = '(';
					  name[i++] = ')';
					  name[i] = '\0';
					  $$ = newFuncVal((Value*)$1, $2, name, lineno);
					  if (repl && !parencount[parencountind]) {
					    val = evaluateFuncVal($$);
					    freeValue((Value*)$$);
					    if (val == NULL)
					      YYABORT;
					    s = valueToString(val);
					    printf("=> %s\n", s);
					    free(s);
					    $$ = (FuncVal*)val;
					    printf(">>> ");
					  }
					}
		;
funcdef		: '[' list ']' '{' statementlist '}'	{
							  if (!repl) {
							    printFunc($2, $5);
							  }
							  $$ = newValue('a', newFuncDef($2, $5, 0));
							  deleteFromListData(parseTreeList, $5);
							}
		| '[' ']' '{' statementlist '}'		{
							  if (!repl) {
							    printFunc(NULL, $4);
							  }
							  $$ = newValue('a', newFuncDef(NULL, $4, 0));
							  deleteFromListData(parseTreeList, $4);
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
		| value			{
					  $$ = newList();
					  $$->next = newList();
					  addToListEnd($$->next, $1);
					}

		| list ',' range	{
					  if ($1->next == NULL)
					    $1->next = newList();
					  addToListEnd($1->next, $1);
					  $$ = $1;
					}
		| range			{
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
		;
range		: value '.' '.' value '.'  '.' value	{
					  $$ = newRange($1, $7, $4);
					}
		| value '.' IND '.' '.' value	{
					  double* d;
					  Value* inc;
					  d = malloc(sizeof(double));
					  *d = $3;
					  inc = newValue('n', d);
					  $$ = newRange($1, $6, inc);
					}
		| value '.' '.' value '.' IND	{
					  double* d;
					  Value* end;
					  d = malloc(sizeof(double));
					  *d = $6;
					  end = newValue('n', d);
					  $$ = newRange($1, end, $4);
					}
		| value '.' IND '.' IND	{
					  double* d, *e;
					  Value* end, *inc;
					  d = malloc(sizeof(double));
					  e = malloc(sizeof(double));
					  *d = $3;
					  *e = $5;
					  inc = newValue('n', d);
					  end = newValue('n', e);
					  $$ = newRange($1, end, inc);
					}
		| value '.' '.' value	{
					  $$ = newRange($1, $4, NULL);
					}
		| value '.' IND		{
					  double* d;
					  Value* end;
					  d = malloc(sizeof(double));
					  *d = $3;
					  end = newValue('n', d);
					  $$ = newRange($1, end, NULL);
					}
		;
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
					  free($3);
					}
		| compexpr '|' compexpr
					{
					  $$ = $1;
					  addToListEnd($$->stack, or);
					  addListToList($$->stack, $3->stack);
					  free($3);
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
				  v = newValue('x', s);
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
				  List* node;
				  n = malloc(sizeof(double));
				  *n = $1;
				  if (!repl) {
				    for (node = numlist; node != NULL; node = node->next) {
				      if (node->data && *(double*)node->data == *n) {
					break;
				      }
				    }
				    if (node == NULL)
				      addToListEnd(numlist, n);
				  }
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
				  deleteFromListData(parseTreeList, $2);
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
  const int constoffsets[] = {38, 44, 48, 54, 62, 68, 74, 80, 86,
	92, 98, 104, 110, 116, 122, 128, 134}; /* 140 next */
  int i;
  Value* v;
  List* sl;
  char* s;
  /* if (argc != 2) { */
  /*   printf("This program takes exactly one argument. The file to interpret\n"); */
  /*   return 1; */
  /* } */
  /* Number of bytes needed to store all the chars in str,
   * with alignment padding */
  lenconstants = 140;
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
  if (argc-1) {
    argfilename = malloc(strlen(argv[1])+1);
    i = 0;
    while ((argfilename[i] = argv[1][i]))
      i++;
    fp = fopen(argfilename, "r");
    yyin = fp;
    if (!yyin) {
      errmsg("File does not exist.");
      i = 1;
      cleanupFffll(NULL);
      return 1;
    }
  } else {
    yyin = NULL;
    repl = 1;
  }
  falsevalue = newValue('0', NULL);
  varnames = newList();
  varlist = newList();
  stringlist = newList();
  numlist = newList();
  constlist = newList();
  jmplist = newList();
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
  funcdeftable[16] = newValue('a', newBuiltinFuncDef(&saveDef, 0));
  /* don't forget to fix funcdeftable's declaration */
  globalvars = newTree(constants+14, stdfiles[0]);
  globalvars = insertInTree(globalvars, constants+15, stdfiles[0]);
  globalvars = insertInTree(globalvars, constants+22, stdfiles[1]);
  globalvars = insertInTree(globalvars, constants+23, stdfiles[1]);
  globalvars = insertInTree(globalvars, constants+30, stdfiles[2]);
  globalvars = insertInTree(globalvars, constants+31, stdfiles[2]);
  addToListBeginning(varnames, constants+14);
  addToListBeginning(varnames, constants+15);
  addToListBeginning(varnames, constants+22);
  addToListBeginning(varnames, constants+23);
  addToListBeginning(varnames, constants+30);
  addToListBeginning(varnames, constants+31);
  for (i=0;i<17;i++) {
    globalvars = insertInTree(globalvars, constants+constoffsets[i],   funcdeftable[i]);
    globalvars = insertInTree(globalvars, constants+constoffsets[i]+1, funcdeftable[i]);
    addToListBeginning(varnames, constants+constoffsets[i]);
    addToListBeginning(varnames, constants+constoffsets[i]+1);
  }
  addToListBeginning(varlist, globalvars);
  parencount = malloc(16*sizeof(int));
  parencount[parencountind] = 0;
  signal(SIGABRT, siginfo);
  signal(SIGFPE, siginfo);
  signal(SIGILL, siginfo);
  signal(SIGINT, siginfo);
  signal(SIGTERM, siginfo);
  parseTreeList = newList();
  yyinlist = newList();
  if (!repl) {
    printf(".intel_syntax noprefix\n.text\n.globl _start\n");
  }
    /* printf("%s%s%s%s%s%s%s%s%s%s%s", */
/* ".intel_syntax noprefix\n" */
/* ".text\n" */
/* "	.globl	_deref_var\n" */
/* "	.globl	_safe_exit\n" */
/* "	.globl	_init_heap\n" */
/* "	.globl	_read\n" */
/* "	.globl	_write\n" */
/* "	.globl	stdin\n" */
/* "	.globl	stdout\n" */
/* "	.globl	stderr\n" */
/* "	.globl	brk\n" */
/* "	.globl	sbrk\n" */
/* "\n" */
/* "\n" */
/* "_allocstr:\n" */
/* "	push rbx\n" */
/* "	mov rbx, [sbrk]\n" */
/* "	add rbx, 32\n" */
/* "	cmp rbx, [brk]\n" */
/* "	jbe __allocstr_room\n" */
/* "	add rbx, 2016\n" */
/* "	mov rax, 0x0c\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rsi, rcx\n" */
/* "	syscall\n" */
/* "	sub rbx, 2016\n" */
/* "	cmp rax, [brk]\n" */
/* "	jbe _safe_exit		# Out of Memory\n", */
/* "	mov [brk], rax\n" */
/* "__allocstr_room:\n" */
/* "	mov rax, [sbrk]\n" */
/* "	mov [sbrk], rbx\n" */
/* "	pop rbx\n" */
/* "	ret\n" */
/* "\n" */
/* "\n" */
/* "_init_heap:\n" */
/* "	mov rax, 0x0c\n" */
/* "	xor rdx, rdx\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rsi, rcx\n" */
/* "	syscall\n" */
/* "	mov [sbrk], rax\n" */
/* "	mov rbx, rax\n" */
/* "	add rbx, 2048\n" */
/* "	mov rax, 0x0c\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rsi, rcx\n" */
/* "	syscall\n" */
/* "	cmp rax, rbx\n" */
/* "	jb _safe_exit\n" */
/* "	mov [brk], rax\n" */
/* "	ret\n" */
/* "\n", */
/* "\n" */
/* "_freestr:\n" */
/* "	push rax\n" */
/* "__freestr_overwrite:\n" */
/* "	mov byte ptr [rax], 0\n" */
/* "	inc rax\n" */
/* "	cmp byte ptr [rax], 0\n" */
/* "	jne __freestr_overwrite\n" */
/* "__freestr_clearblock:\n" */
/* "	inc rax\n" */
/* "	cmp rax, sbrk\n" */
/* "	jae __freestr_shrink\n" */
/* "	cmp byte ptr [rax], 0\n" */
/* "	je __freestr_clearblock\n" */
/* "__freestr_ret:\n" */
/* "	pop rax\n" */
/* "	ret\n" */
/* "__freestr_shrink:\n" */
/* "	pop rax\n" */
/* "	mov [sbrk], rax\n" */
/* "	xor rax, rax\n" */
/* "	ret\n" */
/* "\n", */
/* "\n" */
/* "_reallocstr:\n" */
/* "	push rbx\n" */
/* "	push rax\n" */
/* "	push rax\n" */
/* "	mov rcx, 32\n" */
/* "__reallocstr_readstr:\n" */
/* "	inc rax\n" */
/* "	cmp byte ptr [rax], 0\n" */
/* "	jne __reallocstr_readstr\n" */
/* "	mov rbx, [sbrk]\n" */
/* "__reallocstr_clearblock:\n" */
/* "	cmp rax, rbx\n" */
/* "	je __reallocstr_extend\n" */
/* "	inc rax\n" */
/* "	cmp byte ptr [rax], 0\n" */
/* "	je __reallocstr_clearblock\n" */
/* "__reallocstr_move:\n" */
/* "	add rcx, rax\n" */
/* "	pop rax\n" */
/* "	sub rcx, rax\n" */
/* "	push rbx\n", */
/* "__reallocstr_extend:\n" */
/* "	add rbx, rcx\n" */
/* "	cmp rbx, [brk]\n" */
/* "	jbe __reallocstr_room\n" */
/* "	mov rax, 0x0c\n" */
/* "	add rbx, 2048\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rsi, rcx\n" */
/* "	syscall\n" */
/* "	sub rbx, 2048\n" */
/* "	cmp rax, [brk]\n" */
/* "	jbe _safe_exit		# Out of Memory\n" */
/* "	mov [brk], rax\n" */
/* "__reallocstr_room:\n" */
/* "	pop rax\n" */
/* "	pop rcx\n" */
/* "	push rax\n" */
/* "	cmp rcx, rax\n" */
/* "	je __reallocstr_ret\n" */
/* "__reallocstr_copyloop:\n" */
/* "	cmp rax, [brk]\n" */
/* "	je __reallocstr_ret\n" */
/* "	cmp byte ptr [rcx], 0\n" */
/* "	je __reallocstr_ret\n" */
/* "	mov dl, [rcx]\n" */
/* "	mov [rax], dl\n" */
/* "	mov byte ptr [rcx], 0\n" */
/* "	inc rax\n" */
/* "	inc rcx\n" */
/* "	jmp __reallocstr_copyloop\n", */
/* "__reallocstr_ret:\n" */
/* "	pop rax\n" */
/* "	mov [sbrk], rbx\n" */
/* "	pop rbx\n" */
/* "	ret\n" */
/* "\n" */
/* "\n" */
/* "__deref_var_once:\n" */
/* "	inc rax\n" */
/* "	mov rax, [rax]\n" */
/* "_deref_var:\n" */
/* "	cmp byte ptr [rax], 'v'	# check for variable\n" */
/* "	je __deref_var_once\n" */
/* "	ret\n" */
/* "\n" */
/* "\n" */
/* "_safe_exit:\n" */
/* "	mov rdi, rax	# move return value\n" */
/* "	mov rax,60	# sys_exit\n" */
/* "	syscall\n" */
/* "\n" */
/* "\n" */
/* "_pop:\n" */
/* "	cmp byte ptr [rax], 'l'\n" */
/* "	jne _safe_exit\n" */
/* "	push rax\n" */
/* "\n", */
/* "\n" */
/* "_push:\n" */
/* "	cmp byte ptr [rax], 'l'\n" */
/* "	jne _safe_exit\n" */
/* "	mov rax, [rax+4]\n" */
/* "	call _list_push\n" */
/* "	ret\n" */
/* "\n" */
/* "_read:\n" */
/* "	## cmp byte ptr [rax], 'h'\n" */
/* "	## je __read_http		# check for http stream\n" */
/* "	cmp byte ptr [rax], 'f'\n" */
/* "	jne _safe_exit		# check for file stream\n" */
/* "	xor rbx, rbx\n" */
/* "	mov [rnb], rbx\n" */
/* "	mov rbx, rax\n" */
/* "	add rbx, 4\n" */
/* "	mov rbx, [rbx]\n" */
/* "	mov rdx, 32\n" */
/* "	call _allocstr\n" */
/* "	mov rcx, rax\n" */
/* "	push rcx\n" */
/* "__read_fillbuf:\n" */
/* "	mov rax, 0\n" */
/* "	mov rdx, 32\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rsi, rcx\n" */
/* "	syscall\n", */
/* "	mov rcx, rsi\n" */
/* "	push rcx\n" */
/* "	cmp rax, 0\n" */
/* "	jl _safe_exit\n" */
/* "	je __read_ret\n" */
/* "	mov rdx, rax\n" */
/* "__read_nl:\n" */
/* "	cmp byte ptr [rcx], 10\n" */
/* "	je __read_clearbuf\n" */
/* "	inc rcx\n" */
/* "	dec rdx\n" */
/* "	cmp rdx, 0\n" */
/* "	jne __read_nl\n" */
/* "	pop rax\n" */
/* "	push rcx\n" */
/* "	call _reallocstr\n" */
/* "	pop rcx\n" */
/* "	jmp __read_fillbuf\n" */
/* "__read_clearbuf:\n" */
/* "	cmp rdx, 0\n" */
/* "	je __read_ret\n" */
/* "	mov byte ptr [rcx], 0\n" */
/* "	inc rcx\n" */
/* "	dec rdx\n" */
/* "	jmp __read_clearbuf\n" */
/* "__read_ret:\n" */
/* "	pop rax\n" */
/* "	pop rax\n" */
/* "	ret\n" */
/* "\n" */
/* "\n" */
/* "_write:\n", */
/* "	mov rdx, rax\n" */
/* "	mov rax, rbx\n" */
/* "	## Check for 'f' or 'h'\n" */
/* "	# cmp byte ptr [rdx], 'h'\n" */
/* "	# je _write_http\n" */
/* "	cmp byte ptr [rdx], 'f'\n" */
/* "	jne _safe_exit\n" */
/* "	add rdx, 4\n" */
/* "	xor rbx, rbx\n" */
/* "	mov [rnb], rbx\n" */
/* "	mov bl, [rdx]\n" */
/* "	xor rdx, rdx\n" */
/* "	call _deref_var\n" */
/* "	cmp byte ptr [rax], 'n'	# check for number\n" */
/* "	jne __write_not_num\n" */
/* "	push rbx\n" */
/* "	push rax\n" */
/* "	call _allocstr\n" */
/* "	mov rbx, rax\n" */
/* "	mov [rnb], rax\n" */
/* "	pop rax\n" */
/* "	push rbx\n" */
/* "	call _numtostr		# go to number routine\n" */
/* "	pop rax\n" */
/* "	pop rbx\n" */
/* "	jmp __write_syscall\n" */
/* "__write_not_num:\n", */
/* "	cmp byte ptr [rax], 's'\n" */
/* "	jne _safe_exit\n" */
/* "	add rax, 4\n" */
/* "	mov rax, [rax]\n" */
/* "__write_syscall:\n" */
/* "	mov rcx, rax\n" */
/* "__write_str:\n" */
/* "	inc rdx			# increase length\n" */
/* "	inc rax			# look at next character\n" */
/* "	cmp byte ptr [rax], 0	# compare with null\n" */
/* "	jne __write_str		# write it if it isn't null\n" */
/* "	mov rsi, rcx\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rax, 1		# sys_write\n" */
/* "	syscall\n" */
/* "	mov rcx, rsi\n" */
/* "	dec rdx\n" */
/* "	add rcx, rdx\n" */
/* "	cmp byte ptr [rcx], 10\n" */
/* "	je __write_ret\n" */
/* "__write_stdout_nl:\n" */
/* "	cmp rbx, 0x0001\n" */
/* "	je __write_newline\n" */
/* "	cmp rbx, 0x0002\n" */
/* "	je __write_newline\n", */
/* "	mov rax, 1\n" */
/* "	ret\n" */
/* "__write_newline:\n" */
/* "	mov rcx, offset rnb	# use write buf\n" */
/* "	cmp byte ptr [rcx], 0\n" */
/* "	jne __write_freestr\n" */
/* "__write_freestr_ret:\n" */
/* "	mov byte ptr [rcx], 10	# store newline\n" */
/* "	mov rdx, 1	# len 1\n" */
/* "	mov rdi, rbx\n" */
/* "	mov rsi, rcx\n" */
/* "	mov rax, 1	# sys_write\n" */
/* "	syscall\n" */
/* "__write_ret:\n" */
/* "	ret\n" */
/* "\n" */
/* "__write_freestr:\n" */
/* "	mov rax, [rcx]\n" */
/* "	call _freestr\n" */
/* "	jmp __write_freestr_ret\n" */
/* "\n"); */
  /* } */
  yyparse();
  free(parencount);
  v = NULL;
  i = 0;
  if (repl) {
    if (lineno-1)
      v = ((List*)parseTreeList->data)->data;
  } else {
    printf("_start:\ncall _init_heap\ncall _init_list\n");
    /* v = evaluateStatements(parseTreeList->data); */
    localvarlist = newList();
    for (sl = parseTreeList->data;sl != NULL;sl = sl->next) {
      if (sl->data) {
	printf("%s\n", (s = valueToLlvmString(sl->data, NULL, localvarlist)));
	free(s);
      }
    }
    freeList(localvarlist);
    printf("call _safe_exit\n\n");
    printf(".data\n"
	   /* "stdin:	.long 0x66, 0x0, 0x0\n" */
	   /* "stdout:	.long 0x66, 0x1, 0x0\n" */
	   /* "stderr:	.long 0x66, 0x2, 0x0\n" */
	   );
    i = 0;
    for (sl = stringlist;sl != NULL;sl = sl->next) {
      if (sl->data) {
	printf("strlist_%d:	.long 0x73\n		.quad .+8\n		.byte %s\n", i, (s = stringToHexString(((String*)sl->data)->val)));
	i++;
	free(s);
      }
    }
    i = 0;
    for (sl = numlist; sl != NULL;sl = sl->next) {
      if (sl->data) {
	printf("numlist_%d:	.long 0x6e, %s\n", i, (s = doubleToHexString(sl->data)));
	i++;
	free(s);
      }
    }
    printf(".bss\n"
	   /* "	.lcomm	rnb	4\n" */
	   /* "	.lcomm	brk	8\n" */
	   /* "	.lcomm	sbrk	8\n" */
	   );
    for (sl = varnames; sl != NULL; sl = sl->next) {
      if (sl->data && ((char*)sl->data < constants || constants+140 < (char*)sl->data)) {
	if ((*(char*)sl->data) == '_')
	  printf(".lcomm const_%s 12\n", (char*)sl->data);
	else
	  printf(".lcomm var_%s 12\n", (char*)sl->data);
      }
    }
  }
  cleanupFffll(v);
  return 0;
}
