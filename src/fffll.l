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

#include <libgen.h>
#include "fffll.h"
#include "list.h"
#include "value.h"
#include "tree.h"
#include "evaluate.h"

#undef YY_BUF_SIZE
#define YY_BUF_SIZE 128

int lineno = 1;
List* buffernames = NULL;
int addTreeKeysToList(List* l, VarTree* vt);
extern int *parencount;
extern int parencountind;
extern int parencountmax;
extern YYSTYPE yylval;
extern int yywrap(void) { return 1; }
extern void yyerror(const char* msg);
extern List* parseTreeList;
extern List* varlist;
extern Value* falsevalue;
extern char* argfilename;

%}
%%

\/.*\/			{
			  char* s;
			  int i;
			  s = malloc(strlen(yytext - 1));
			  for (i=0;(s[i] = yytext[i+1]);i++) {
			    if (yytext[i+1] == '/')
			      break;
			  }
			  s[i] = '\0';
			  yylval.symbol = s;
			  return RGX;
			}
\"([^\"]*(\"\")*)*\"	{
			  int i, l, j, insert;
			  char* funcname, rawchr, inpchr;
			  i = 0;
			  if (yytext[2] == '\0')
			    l = 0;
			  else {
			    for (l=0;yytext[l] != '\0';l++) {
			      if (yytext[l] == '"') {
			        i++;
			        if (yytext[l+1] == '"')
			          l++;
			      }
			    }
			    l -= i;
			  }
			  l++;
			  funcname = malloc(l);
			  l--;
			  j = 1;
			  for (i=0;i<l;i++) {
			    if (yytext[i+j] == '"' && yytext[i+j+1] == '"')
			      j++;
			    funcname[i] = yytext[i+j];
			  }
			  funcname[i] = '\0';
			  l++;
			  insert = 0;
			  if (parencount[parencountind])
			    insert = 1;
			  while (1) {
			    do {
			      inpchr = input();
			    } while (insert && (inpchr == ' ' || inpchr == '\t' ||
			             inpchr == '\n'));
			    unput(inpchr);
			    if (inpchr == '@') {
			      input();
			      inpchr = input();
			      if (47 < inpchr && inpchr < 58) {
			        inpchr -= 48;
			      } else {
			        if (inpchr > 90)
			          inpchr -= 32;
			        inpchr -= 55;
			      }
			      rawchr = inpchr * 16;
			      inpchr = input();
			      if (47 < inpchr && inpchr < 58) {
			        inpchr -= 48;
			      } else {
			        if (inpchr > 90)
			          inpchr -= 32;
			        inpchr -= 55;
			      }
			      rawchr += inpchr;
			    } else
			      break;
			    do {
			      inpchr = input();
			    } while (insert && (inpchr == ' ' || inpchr == '\t' ||
			             inpchr == '\n'));
			    if (i+2 > l) {
			      l += 2;
			      funcname = realloc(funcname, l);
			    }
			    funcname[i++] = rawchr;
			    funcname[i] = '\0';
			    if (inpchr != '"') {
			      unput(inpchr);
			      if (inpchr == '@')
			        continue;
			      else
			        break;
			    }
			    while ((inpchr = input()) != '"') {
			      if (i == l) {
			        l *= 2;
			        funcname = realloc(funcname, l);
			      }
			      funcname[i++] = inpchr;
			    }
			    if (i == l) {
			      l++;
			      funcname = realloc(funcname, l);
			    }
			    funcname[i] = '\0';
			  }
			  if (inpchr != '{' && inpchr != '[' && inpchr != '"') /* " */
			    insert = 0;
			  if (insert) {
			    unput(',');
			  }
			  yylval.symbol = funcname;
			  return STR;
			}
%			{
			  int i, j, k;
			  char* filename, *foldername, c;
			  Variable* var;
			  FILE* oldyyin;
			  foldername = dirname(argfilename);
			  k = strlen(foldername) + 2;
                          i = 0;
			  filename = malloc(k);
			  while ((filename[i] = foldername[i]))
			    i++;
			  filename[i++] = '/';
			  j = i;
			  while ((c = input()) != ',' &&
			         c != '\n') {
			    filename[i++] = c;
			    if (i == k) {
			      k *= 2;
			      filename = realloc(filename, k);
			    }
			  }
			  if (c == ',')
			    unput('%');
			  if (i+4 >= k) {
			    k += 4;
			    filename = realloc(filename, k);
			  }
			  filename[i] = '.';
			  filename[i+1] = 'f';
			  filename[i+2] = 'f';
			  filename[i+3] = '\0';
			  oldyyin = yyin;
			  yyin = fopen(filename, "r");
			  if (!yyin) {
			    j++;
			    foldername = getenv("FFFLL_DIR");
			    if (!foldername) {
			      yyerror("FFFLL_DIR environment variable is not set! Can't find import");
			      free(filename);
			    } else {
			      i = strlen(foldername);
			      if (i>j) {
				j = i - j;
				filename = realloc(filename, k+j+1);
				for (;k>i-j-1;k--) {
				     filename[k+j] = filename[k];
				}
				j = i - j;
			      } else if (j>i) {
			        j -= i;
				k = i;
				while (1) {
				  filename[k] = filename[k+j];
				  if (!filename[k])
				    break;
				  k++;
				}
				j += i;
			      }
			      for (k=0;k<i;k++) {
				filename[k] = foldername[k];
			      }
			      i = strlen(filename) - 3;
			      j--;
			      yyin = fopen(filename, "r");
			      if (!yyin) {
			        yyerror(filename);
				free(filename);
			      }
			    }
			  }
			  if (yyin) {
			    filename[i] = '\0';
			    foldername = malloc(strlen(filename+j)+1);
			    i = 0;
			    while ((foldername[i]  = filename[j+i]))
			      i++;
			    free(filename);
			    var = parseVariable(foldername);
			    /*addToListBeginning(parseTreeList, newList());*/
			    if (buffernames == NULL) {
			      buffernames = newList();
			      buffernames->data = var->name;
			    } else {
			      addToListBeginning(buffernames, var->name);
			    }
			    freeVariable(var);
			    yypush_buffer_state(yy_create_buffer( yyin, YY_BUF_SIZE));
			  } else {
			    yyin = oldyyin;
			  }
			}
[,.:()[\]\{\}<>=&|!?~] {
			  int insert = 0;
			  char inschr = ',', inpchr;
			  if (yytext[0] == '(')
			    parencount[parencountind]++;
                          else if (yytext[0] == ')')
			    parencount[parencountind]--;
			  else if (yytext[0] == '{') {
			    parencountind++;
			    if (parencountind == parencountmax) {
			      parencountmax *= 2;
			      parencount = realloc(parencount,
			                           parencountmax*sizeof(int));
			    }
			    parencount[parencountind] = 0;
			  } else if (yytext[0] == '}') {
			    parencountind--;
			  }
			  if (parencount[parencountind] && (yytext[0] == ')' || yytext[0] == '}' || yytext[0] == ']')) {
			    insert = 1;
			    do {
			      inpchr = input();
			    } while (inpchr == ' ' || inpchr == '\t' ||
			             inpchr == '\n');
			    unput(inpchr);
			    if (inpchr != '{' && inpchr != '[' && inpchr != '"')
			      insert = 0;
			    else if (yytext[0] == ']' && inpchr == '{')
			      insert = 0;
			  }
			  if (insert) {
			    unput(inschr);
			  }
			  return yytext[0];
			}
-?[0-9]+(\.[0-9]+)?	{
			  yylval.num = atof((char*)yytext);
			  return NUM;
			}
\.-?[0-9]+		{
			  yylval.index = atoi((char*)yytext+1);
			  return IND;
			}
[_a-zA-Z][_A-Za-z0-9]*	{
			  int i, l;
			  char* funcname;
			  l = strlen(yytext);
			  funcname = malloc(l+1);
			  for (i=0;i<l;i++) {
			    funcname[i] = yytext[i];
			  }
			  funcname[l] = '\0';
			  yylval.symbol = funcname;
			  return VAR;
			}
--			{
			  char c;
			  if ((c = input()) != '\n') {
			    if (c == '*') {
			      while (1) {
			        while ((c = input()) != '*' && c > -1)
				  if (c == '\n') lineno++;
			        if ((c = input()) == '-') {
			          if ((c = input()) == '-') {
				    break;
				  } else if (c == '\n') {
				    lineno++;
                                  }
                                } else if (c == '\n') {
				  lineno++;
				}
				if (c == -1)
				  break;
			      }
			    } else {
			      while ((c = input()) != '\n' && c > -1);
			    }
			  }
			}
[ \t]			;
\n			{
			  lineno++;
			}
<<EOF>>			{
			  List* results;
			  Value* v;
			  fclose(yyin);
			  yypop_buffer_state();
			  if (!YY_CURRENT_BUFFER) {
			    yyterminate();
			  } else if (parseTreeList->next != NULL) {
			    addToListBeginning(varlist, NULL);
			    v = evaluateStatements(parseTreeList->data);
			    evaluateValue(v);
			    if (v != NULL && v != falsevalue) {
			      freeValue(v);
			    }
			    results = newList();
			    results->data = newList();
			    ((List*)results->data)->data = varlist->data;
			    addTreeKeysToList(results->data, varlist->data);
			    varlist->next->data = insertInTree(varlist->next->data, buffernames->data, newValue('l', results));
			    varlist = deleteFromListBeginning(varlist);
			    /*parseTreeList = deleteFromListBeginning(parseTreeList);*/
			    buffernames = deleteFromListBeginning(buffernames);
			  }
			}
.			{
                          char errmsg[22];
			  snprintf(errmsg, 22, "Illegal character '%c'", yytext[0]);
			  yyerror(errmsg);
			}
%%

int addTreeKeysToList(List* l, VarTree* vt) {
  if (vt == NULL)
    return 0;
  addToListEnd(l, vt->key);
  addTreeKeysToList(l, vt->left);
  addTreeKeysToList(l, vt->right);
  return 0;
}
