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

/* #include <stdio.h> */
/* #include <errno.h> */

#include "fffll.h"
#include <libgen.h>
#include "list.h"
#include <stdlib.h>
#include <string.h>
#include "tree.h"
#include "value.h"

#include <readline/readline.h>
#include <readline/history.h>

int lineno = 1;
List* buffernames = NULL;
int addTreeKeysToList(List* l, VarTree* vt);
char* readlinebuf = NULL;
int readlinebufpos;

FILE *yyin;
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
extern List* yyinlist;

int getfffll() {
  if (yyin)
    return getc(yyin);
  if (readlinebuf == NULL || readlinebuf[readlinebufpos] == '\0') {
    if (readlinebuf) {
      lineno++;
      free(readlinebuf);
    }
    readlinebuf = readline(">>> ");
    if (readlinebuf && *readlinebuf)
      add_history(readlinebuf);
    readlinebufpos = 0;
  }
  if (!readlinebuf)
    return EOF;
  return readlinebuf[readlinebufpos++];
}

int ungetfffll(int c) {
  if (yyin)
    return ungetc(c, yyin);
  else
    return (readlinebuf[--readlinebufpos] = c);
}

int gethex() {
  int c;
  c = getfffll();
  if (47 < c && c < 58) {
    c -= 48;
  } else {
    if (c > 90)
      c -= 32;
    c -= 55;
  }
  return c;
}

int yylex(void) {
  char *s, *folder, c, endc, inp;
  int i = 0, l = 32, d, j;
  Variable* var;
  FILE* oldyyin;
  do {
    c = getfffll();
    if (c == EOF) {
      goto handleEOF;
    }
    if (c == '\n')
      lineno++;
    if (c == '-') {
      c = getfffll();
      if (c != '-') {
	ungetfffll(c);
	c = '-';
      } else {
	c = getfffll();
	if (c == '\n') {
	  lineno++;
	  break;
	}
	if (c != '*') {
	  if (c != '\n') {
	    while ((c = getfffll()) != '\n');
	  }
	  lineno++;
	} else {
	  while (1) {
	    while ((c = getfffll()) != '*')
	      if (c == EOF) goto handleEOF;
	    if (c == '\n') lineno++;
	    if ((c = getfffll()) == '-') {
	      if ((c = getfffll()) == '-') {
		c = getfffll();
		break;
	      } else {
		if (c == '\n') lineno++;
	      }
	    } else {
	      if (c == '\n') lineno++;
	    }
	  }
	}
      }
    }
  } while (c == '\t' || c == ' ' || c == '\n');
  switch (c) {
  case '/':
  case '"':
    s = malloc(l);
    endc = c;
    /* don't forget funny string stuff... */
    d = 1;
    do {
      s[i] = getfffll();
      if (s[i] == endc) {
	d = 0;
	j = i - 1;
	if (endc == '/')
	  while (s[j--] == '\\' && j > -1)
	    d = !d;
	if (endc == '"') {
	  while ((c = getfffll()) == '@') {
	    s[i++] = gethex() * 16 + gethex();
	  }
	  if (c == '"') {
	    d = 1;
	    i--;
	  } else {
	    ungetfffll(c);
	  }
	}
      }
      if (i+1 == l) {
	l *= 2;
	s = realloc(s, l);
      }
      i++;
    } while (d);
    s[--i] = '\0';
    yylval.symbol = s;
    if (endc == '/')
      return RGX;
    if (endc == '"') {
      if (parencount[parencountind] && (c == ')' || c == '}' || c == ']')) {
	do {
	  inp = getfffll();
	  if (inp == '\n')
	    lineno++;
	} while (inp == ' ' || inp == '\t' || inp == '\n');
	ungetfffll(inp);
	if (inp == '[' || inp == '"' || inp == '{')
	  ungetfffll(',');
      }
      return STR;
    }
    break;
  case '%':
    folder = dirname(argfilename);
    l += strlen(folder) + 2;
    s = malloc(l);
    while ((s[i] = folder[i]))
      i++;
    s[i++] = '/';
    j = i;
    while ( (c = getfffll()) != ',' && c != '\n') {
      s[i++] = c;
      if (i == l) {
	l *= 2;
	s = realloc(s, l);
      }
    }
    if (c == ',')
      ungetfffll('%');
    if (i+4 >= l) {
      l += 4;
      s = realloc(s, l);
    }
    s[i] = '.';
    s[i+1] = 'f';
    s[i+2] = 'f';
    s[i+3] = '\0';
    oldyyin = yyin;
    yyin = fopen(s, "r");
    if (!yyin) {
      j++;
      folder = getenv("FFFLL_DIR");
      if (!folder) {
	yyerror("FFFLL_DIR environment variable is not set! Can't find import");
	free(s);
	s = NULL;
      } else {
	i = strlen(folder);
	if (i>j) {
	  j = i - j;
	  s = realloc(s, l+j+1);
	  for (;l>i-j-1;l--)
	    s[l+j] = s[l];
	  j = i - j;
	} else if (j>i) {
	  j -= i;
	  l = i;
	  while (1) {
	    s[l] = s[l+j];
	    if (!s[l])
	      break;
	    l++;
	  }
	  j += i;
	}
	for (l=0;l<i;l++)
	  s[l] = folder[l];
	i = strlen(s) - 3;
	j--;
	yyin = fopen(s, "r");
	if (!yyin) {
	  yyerror(s);
	  free(s);
	  s = NULL;
	}
      }
    }
    if (!yyin) {
      yyin = oldyyin;
    } else {
      s[i] = '\0';
      folder = malloc(strlen(s+j)+1);
      i = 0;
      while( (folder[i] = s[j+i]) )
	i++;
      free(s);
      s = NULL;
      var = parseVariable(folder);
      addToListBeginning(parseTreeList, newList());
      if (buffernames == NULL) {
	buffernames = newList();
	buffernames->data = var->name;
      } else {
	addToListBeginning(buffernames, var->name);
      }
      freeVariable(var);
      addToListBeginning(yyinlist, oldyyin);
    }
    return yylex();
  case '.':
    s = malloc(l);
    s[i] = getfffll();
    if (s[i] == '-') {
      s[++i] = getfffll();
    }
    if (47 < s[i] && s[i] < 58) {
      do {
	if (i == l - 1) {
	  l *= 2;
	  s = realloc(s, l);
	}
	s[++i] = getfffll();
      } while (47 < s[i] && s[i] < 58);
      ungetfffll(s[i]);
      s[i] = '\0';
      yylval.index = atoi(s);
      free(s);
      return IND;
    } else {
      while (i > -1) {
	ungetfffll(s[i]);
	i--;
      }
      free(s);
      i = 0;
    }
  case ',':
  case ':':
  case '(':
  case ')':
  case '[':
  case ']':
  case '{':
  case '}':
  case '<':
  case '>':
  case '=':
  case '&':
  case '|':
  case '!':
  case '?':
  case '~':
    if (c == '(')
      parencount[parencountind]++;
    else if (c == ')')
      parencount[parencountind]--;
    else if (c == '{') {
      parencountind++;
      if (parencountind == parencountmax) {
	parencountmax *= 2;
	parencount = realloc(parencount,
			     parencountmax*sizeof(int));
      }
      parencount[parencountind] = 0;
    } else if (c == '}') {
      parencountind--;
    }
    if (parencount[parencountind] && (c == ')' || c == '}' || c == ']')) {
      do {
	inp = getfffll();
	if (inp == '\n')
	  lineno++;
      } while (inp == ' ' || inp == '\t' || inp == '\n');
      ungetfffll(inp);
      if (inp == '[' || inp == '"' || (inp == '{' && c != ']'))
	ungetfffll(',');
    }
    return c;
  case '-':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    s = malloc(l);
    s[i] = c;
    do {
      if (s[i] == '.')
	d = 1;
      s[++i] = getfffll();
      if (i+1 == l) {
	l *= 2;
	s = realloc(s, l);
      }
    } while ((47 < s[i] && s[i] < 58)
	     || (!d && s[i] == '.'));
    ungetfffll(s[i]);
    if (s[i-1] == '.')
      ungetfffll(s[--i]);
    s[i] = '\0';
    yylval.num = atof(s);
    free(s);
    return NUM;
  default:
    s = malloc(l);
    s[i] = c;
    do {
      s[++i] = getfffll();
      if (i+1 == l) {
	l *= 2;
	s = realloc(s, l);
      }
    } while (s[i] != '\t' && s[i] != '\n' && s[i] != ' ' && s[i] != ':' &&
	     s[i] != '(' && s[i] != '.' && s[i] != '<' && s[i] != '=' &&
	     s[i] != '>' && s[i] != '?' && s[i] != '~' && s[i] != '&' &&
	     s[i] != '|' && s[i] != ')' && s[i] != ',' && s[i] != ']');
    ungetfffll(s[i]);
    s[i] = '\0';
    yylval.symbol = s;
    return VAR;
  }
 handleEOF:
  if (yyin)
    fclose(yyin);
  yyin = yyinlist->data;
  if (yyin == NULL || parseTreeList->next == NULL)
    return 0;
  yyinlist = deleteFromListBeginning(yyinlist);
  yylval.symbol = buffernames->data;
  buffernames = deleteFromListBeginning(buffernames);
  return EOFSYM;
}
