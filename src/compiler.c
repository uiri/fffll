#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "tree.h"

extern char* constants;
extern char* anonfunc;
extern List* stringlist;

char* valueToLlvmString(Value* v, int stmnt) {
  char* s, *t, *strptr = "__fffll_strlist", *underscore = "const ";
  Variable* var;
  FuncDef* fd;
  FuncVal* fv;
  BoolExpr* be;
  List* node;
  int i = 0, j, k, m, n;
  k = 0;
  switch (v->type) {
  case 'v':
    var = (Variable*)v;
    m = 0;
    if (var->name[0] == '_' && stmnt)
      m = 6;
    if (var->indextype[0] != '0') {
      j = 256;
      s = malloc(j);
    } else {
      s = malloc(strlen(var->name) + m + 1);
    }
    if (var->name[0] == '_' && stmnt)
      while (( s[i] = underscore[i] )) i++;
    while (( s[i] = var->name[i-m] )) i++;
    for (m=0;var->indextype[m] != '0';m++) {
      t = NULL;
      switch (var->indextype[m]) {
      case 'v':
	k = snprintf(s+i, j-i,"[%s]", (t = valueToLlvmString(var->index[m], 0)));
	break;
      case 'n':
	k = snprintf(s+i, j-i,"[%d]", *(int*)var->index[m]);
	break;
      default:
	k = snprintf(s+i, j-i, "[\"%s\"]", (char*)var->index[m]);
      }
      if (k+i >= j) {
	j *= 2;
	s = realloc(s, j);
	switch (var->indextype[m]) {
	case 'v':
	  k = snprintf(s+i, j-i,"[%s]", t);
	  break;
	case 'n':
	  k = snprintf(s+i, j-i,"[%d]", *(int*)var->index[m]);
	  break;
	default:
	  k = snprintf(s+i, j-i, "[\"%s\"]", (char*)var->index[m]);
	}
      }
      i += k;
      free(t);
    }
    break;
  case 'a':
    fd = v->data;
    j = 256;
    s = malloc(j);
    snprintf(s+i, j-i, "function(options) {\n");
    i += 20;
    if (fd->arguments && fd->arguments->next) {
      n = 0;
      for (node = fd->arguments->next;node != NULL; node = node->next) {
	t = valueToLlvmString(node->data, 0);
	k = snprintf(s+i, j-i, "var %s = options.%s || options['%d'];\n", t, t, n);
	if (k+i >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  k = snprintf(s+i, j-i, "var %s = options.%s || options['%d'];\n", t, t, n);
	}
	i += k;
	n++;
	free(t);
      }
    }
    for (node = fd->statements;node != NULL; node = node->next) {
      k = snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(node->data, 1)));
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "%s\n", t);
      }
      i += k;
      free(t);
    }
    snprintf(s+i, j-i, "}");
    break;
  case 'c':
    fv = (FuncVal*)v;
    j = 256;
    s = malloc(j);
    if (fv->name == constants+39) {
      k = snprintf(s+i, j-i, "%s = ", (t = valueToLlvmString(fv->arglist->next->data, 1)));
      i += k;
      free(t);
      k = snprintf(s+i, j-i, "%s", (t = valueToLlvmString(fv->arglist->next->next->data, 1)));
      if (i+k >= j) {
	while (i+k >= j)
	  j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "%s", t);
      }
      i += k;
      free(t);
    } else if (fv->name == constants+129) {
      k = snprintf(s+i, j-i, "throw %s", (t = valueToLlvmString(fv->arglist->next->data, 0)));
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "throw %s", t);
      }
      i += k;
      free(t);
    } else if (fv->name == constants+135) {
      var = ((List*)fv->arglist->data)->next->data;
      k = snprintf(s+i, j-i, "try {\n%s} catch (%s) {\nvar __fffll_%s = %s;\n", (t = valueToLlvmString(fv->arglist->next->data, 1)), var->name, var->name, var->name);
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "try {\n%s} catch (%s) {\nvar __fffll_%s = %s;\n", t, var->name, var->name, var->name);
      }
      i += k;
      free(t);
      k = snprintf(s+i, j-i, "%sif (__fffll_%s == %s) throw %s;\n}\n", (t = valueToLlvmString(findInTree(((List*)fv->arglist->data)->data, var->name), 1)), var->name, var->name, var->name);
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "%s\nif (__fffll_%s == %s) throw %s;\n}\n", t, var->name, var->name, var->name);
      }
      i += k;
      free(t);
    } else if (fv->name == constants+45) {
      k = snprintf(s+i, j-i, "if %s {\n", (t = valueToLlvmString(fv->arglist->next->data, 0)));
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	snprintf(s+i, j-i, "if %s {\n", t);
      }
      i += k;
      free(t);
      k = snprintf(s+i, j-i, "%s", (t = valueToLlvmString(fv->arglist->next->next->data, 0)));
      if (i+k > j) {
	j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "%s", t);
      }
      i += k;
      free(t);
      for (node = fv->arglist->next->next->next;node != NULL; node = node->next) {
	snprintf(s+i, j-i, "} else ");
	i += 7;
	if (node->next && node->next->data) {
	  k = snprintf(s+i, j-i, "if %s ", (t = valueToLlvmString(node->data, 0)));
	  if (i+k >= j) {
	    j *= 2;
	    s = realloc(s, j);
	    k = snprintf(s+i, j-i, "if %s ", t);
	  }
	  i += k;
	  free(t);
	  node = node->next;
	}
	k = snprintf(s+i, j-i, "{\n%s", (t = valueToLlvmString(node->data, 0)));
	if (i+k >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  k = snprintf(s+i, j-i, "{\n%s", t);
	}
	i += k;
	free(t);
      }
      snprintf(s+i, j-i, "}\n");
    } else if (fv->name == constants+49) {
      if (fv->arglist->data) {
	var = ((List*)fv->arglist->data)->next->data;
	k = snprintf(s+i, j-i, "var __fffll_list = %s;\nfor (__fffll_%s in __fffll_list) {\nvar %s = __fffll_list[__fffll_%s];\n", (t = valueToLlvmString(findInTree(((List*)fv->arglist->data)->data, var->name), 0)), var->name, var->name, var->name);
	if (i+k >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  k = snprintf(s+i, j-i, "var __fffll_list = %s;\nfor (__fffll_%s in __fffll_list) {\nvar %s = __fffll_list[__fffll_%s];\n", t, var->name, var->name, var->name);
	}
	i += k;
	free(t);
	node = fv->arglist->next;
	if (fv->arglist->next->next) {
	  k = snprintf(s+i, j-i, "if (!%s) break;\n", (t = valueToLlvmString(node->data, 0)));
	  if (i+k >= j) {
	    j *= 2;
	    s = realloc(s, j);
	    k = snprintf(s+i, j-1, "if (!%s) break;\n", t);
	  }
	  i += k;
	  free(t);
	  node = fv->arglist->next->next;
	}
      } else {
	if (fv->arglist->next->next) {
	  k = snprintf(s+i, j-i, "while %s {\n", (t = valueToLlvmString(fv->arglist->next->data, 0)));
	  node = fv->arglist->next->next;
	  if (i+k >= j) {
	    j *= 2;
	    s = realloc(s, j);
	    k = snprintf(s+i, j-i, "while %s {\n", t);
	  }
	} else {
	  k = snprintf(s+i, j-i, "while (1) {\n");
	  node = fv->arglist->next;
	  if (i+k >= j) {
	    j *= 2;
	    s = realloc(s, j);
	    k = snprintf(s+i, j-i, "while (1) {\n");
	  }
	  t = NULL;
	}
	i += k;
	free(t);
      }
      k = snprintf(s+i, j-i, "%s", (t = valueToLlvmString(node->data, 1)));
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	k = snprintf(s+i, j-i, "%s", t);
      }
      i += k;
      free(t);
      snprintf(s+i, j-i, "}\n");
    } else {
      k = 0;
      if (fv->name == anonfunc) {
	k = snprintf(s+i, j-i, "%s({", (t = valueToLlvmString(fv->val, 0)));
	if (i+k >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  k = snprintf(s+i, j-i, "%s({", t);
	}
	free(t);
      } else {
	k = snprintf(s+i, j-i, "%s({", fv->name);
	if (i+k >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  k = snprintf(s+i, j-i, "%s({", fv->name);
	}
      }
      i += k;
      if (fv->arglist && fv->arglist->next) {
	n = 0;
	for (node = fv->arglist->next;node != NULL; node = node->next) {
	  t = valueToLlvmString(node->data, 0);
	  k = snprintf(s+i, j-i, "'%d': %s,", n, t);
	  if (i+k >= j) {
	    j *= 2;
	    s = realloc(s, j);
	    k = snprintf(s+i, j-i, "'%d': %s,", n, t);
	  }
	  i += k;
	  n++;
	  free(t);
	}
	i--;
      }
      snprintf(s+i, j-i, "})");
      i++;
      if (stmnt) {
	i++;
	snprintf(s+i, j-i, ";");
      }
    }
    break;
  case 's':
    for (node = stringlist;node != NULL;node = node->next) {
      if (v->data == node->data)
	break;
      i++;
    }
    j = 1;
    k = i;
    while (i > 9) {
      i /= 10;
      j++;
    }
    s = malloc(18+j);
    j += 17;
    i = 0;
    while ((s[i] = strptr[i])) i++;
    s[j--] = '\0';
    s[j--] = ']';
    while (i != j) {
      s[j--] = k%10 + 48;
      k /= 10;
    }
    s[j--] = '[';
    break;
  case 'b':
    be = (BoolExpr*)v;
    j = 256;
    s = malloc(j);
    if (be->neg) {
      snprintf(s+i, j-i, "(!");
      i += 2;
    }
    snprintf(s+i, j-i, "(");
    i++;
    for (node = be->stack;node != NULL; node = node->next) {
      t = node->data;
      if (t[0] == '&' || t[0] == '|' || t[0] == '?' ||
	  t[0] == '>' || t[0] == '<' || t[0] == '=' ||
	  t[0] == '~') {
	snprintf(s+i, j-i, "%c", t[0]);
	i++;
	if (t[0] == '=' || t[0] == '&' || t[0] == '|') {
	  snprintf(s+i, j-i, "%c", t[0]);
	  i++;
	}
      } else {
	snprintf(s+i, j-i, "%s", (t = valueToLlvmString(node->data, 0)));
	i += strlen(t);
	free(t);
      }
    }
    snprintf(s+i, j-i, ")");
    i++;
    if (be->neg) {
      snprintf(s+i, j-i, ")");
    }
    break;
  case 'd':
    j = 256;
    s = malloc(j);
    for (node = v->data; node != NULL; node = node->next) {
      k = snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(node->data, 1)));
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	snprintf(s+i, j-i, "%s\n", t);
      }
      i += k;
      free(t);
    }
    break;
  case 'l':
    j = 256;
    s = malloc(j);
    snprintf(s+i, j-i, "{ ");
    i += 2;
    m = 0;
    if (v->data && (node = ((List*)v->data)->data)) {
      for (node = node->next; node != NULL; node = node->next) {
	var = node->data;
	k = snprintf(s+i, j-i, "%s: %s, ", var->name, (t = valueToLlvmString(findInTree(((List*)((List*)v->data)->data)->data, var->name), 0)));
	if (i+k >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  snprintf(s+i, j-i, "%s: %s, ", var->name, t);
	}
	i += k;
	free(t);
      }
      i -= 2;
    }
    if (v->data) {
      for (node = ((List*)v->data)->next; node != NULL; node = node->next) {
	k = snprintf(s+i, j-i, "%d: %s, ", m++, (t = valueToLlvmString(node->data, 0)));
	if (i+k >= j) {
	  j *= 2;
	  s = realloc(s, j);
	  snprintf(s+i, j-i, "%d: %s, ", m-1, t);
	}
	i += k;
	free(t);
      }
    }
    if (m) {
      i -= 2;
    }
    snprintf(s+i, j-i, "}");
    break;
  case 'r':
    j = 256;
    s = malloc(j);
    k = snprintf(s+i, j-i, "Range()");
    i += k;
    break;
  default:
    s = valueToString(v);
    break;
  }
  return s;
}
