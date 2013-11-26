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

#include <ctype.h>
#include "fffll.y.h"
#include "list.h"

#undef YY_BUF_SIZE
#define YY_BUF_SIZE 128

int lineno = 1;
extern YYSTYPE yylval;
extern int yywrap(void) { return 1; }
extern void yyerror(const char* msg);

%}
%option noinput
%option nounput
%%

\"[^\"]*\"		{
			  int i, l;
			  char* funcname;
			  l = strlen(yytext);
			  l--;
			  funcname = malloc(l);
			  l--;
			  for (i=0;i<l;i++) {
			    funcname[i] = yytext[i+1];
			  }
			  funcname[i] = '\0';
			  yylval.symbol = funcname;
			  return STR;
			}
[,;()[\]\{\}<>=&|!]	{ return yytext[0]; }
-?[0-9]+(\.[0-9]+)?	{
			  yylval.num = atof((char*)yytext);
			  return NUM;
			}
[A-Z][A-Za-z0-9]+	{
			  int i, l;
			  char* funcname;
			  l = strlen(yytext);
			  funcname = malloc(l+1);
			  for (i=0;i<l;i++) {
			    if (yytext[i] > 90) {
			      yytext[i] -= 32;
			    }
			    funcname[i] = yytext[i];
			  }
			  funcname[l] = '\0';
			  yylval.symbol = funcname;
			  return FUNC;
			}
[a-z][A-Za-z0-9]*	{
			  int i, l;
			  char* funcname;
			  l = strlen(yytext);
			  funcname = malloc(l+1);
			  for (i=0;i<l;i++) {
			    if (64 < yytext[i] && yytext[i] < 90) {
			      yytext[i] += 32;
			    }
			    funcname[i] = yytext[i];
			  }
			  funcname[l] = '\0';
			  yylval.symbol = funcname;
			  return VAR;
			}
[ \t]			;
\n			lineno++;
<<EOF>>			{
				fclose(yyin);
				yyterminate();
			}
.			{
			  printf("Illegal character %d ", yytext[0]);
			  yyerror(":(");
			}
