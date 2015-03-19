#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "tree.h"

extern char* constants;
extern char* anonfunc;
extern List* stringlist;
extern List* numlist;
extern List* buffernames;

#define SNPRINTF_REALLOC(stmnt1, stmnt2) k = stmnt1;\
  if (i+k >= j) {\
    while (i+k >= j) {\
      j *= 2;\
      s = realloc(s, j);\
    }\
    k = stmnt2;\
  }\
  i += k

#define BOOL_REALLOC(data) digit = ((Value*)data)->type;\
  if (digit == 'c' || digit == 'b') {\
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nadd rax, 4\nmov rax, [rax]\n", (t = valueToLlvmString(data, prefix, localvars))),\
		     snprintf(s+i, j-i, "%s\nadd rax, 4\nmov rax, [rax]\n", t));\
  } else if (digit == 'n' || digit == 's') {\
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, offset %s\n", (t = valueToLlvmString(data, prefix, localvars))),\
		     snprintf(s+i, j-i, "mov rax, offset %s\n", t));\
    if (digit == 'n') {\
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, rax\nadd rbx, 4\nmov rax, [rbx]\n"),\
		       snprintf(s+i, j-i, "mov rbx, rax\nadd rbx, 4\nmov rax, [rbx]\n"));\
    }\
  } else {\
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s]\nadd rax, 4\nmov rax, [rax]\n", (t = valueToLlvmString(data, prefix, localvars))),\
		     snprintf(s+i, j-i, "mov rax, [%s]\nadd rax, 4\nmov rax, [rax]\n", t));\
  }\
  free(t)

int branchnum = 0;
int blocknum = 0;

void printFunc(List* arglist, List* statementlist) {
  List* node;
  List* localvarlist;
  char* s, *prefix, *prepend, *t;
  int i, j, k;
  printf("_block_%d:\npush rbp\nmov rbp, rsp\n", blocknum);
  blocknum++;
  localvarlist = newList();
  prefix = NULL;
  if (buffernames)
    prefix = buffernames->data;
  i = 16;
  if (arglist) {
    for (node = arglist->next; node != NULL; node = node->next) {
      if (node->data) {
	t = valueToLlvmString(node->data, prefix, localvarlist);
	printf("mov rdx, [%s]\npush rdx\nmov rdx, [rbp+%d]\nmov [%s], rdx\n", t, i, t);
	i += 8;
	free(t);
      }
    }
  }
  for (node = statementlist; node != NULL; node = node->next) {
    if (node->data) {
      printf("%s\n", (s = valueToLlvmString(node->data, prefix, localvarlist)));
      free(s);
    }
  }
  i = 0;
  j = 256;
  s = malloc(j);
  s[0] = '\0';
  if (localvarlist->next != NULL || localvarlist->data != NULL) {
    for (node = localvarlist; node != NULL; node = node->next) {
      prepend = malloc(i+1);
      k = 0;
      while (( prepend[k] = s[k] )) k++;
      if (((char*)((Value*)node->data)->data)[0] == '_') {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\n%s", prepend),
			 snprintf(s+i, j-i, "pop rbx\n%s", prepend));
      } else {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\nmov [%s], rbx\n%s", (t = valueToLlvmString(node->data, prefix, NULL)), prepend),
			 snprintf(s+i, j-i, "pop rbx\nmov [%s], rbx\n%s", t, prepend));
	free(t);
      }
      free(prepend);
    }
  }
  printf("%s", s);
  free(s);
  freeList(localvarlist);
  printf("mov rsp, rbp\npop rbp\nret\n");
}

char* valueToLlvmString(Value* v, char* prefix, List* localvars) {
  char* s, *t, *argpush, *fvname, *strptr = "strlist", *numptr = "numlist", *underscore = "const_", *varprefix = "var_", digit;
  Variable* var;
  FuncDef* fd;
  FuncVal* fv;
  BoolExpr* be;
  List* node;
  int i = 0, j, k, m, n, b, c;
  k = 0;
  switch (v->type) {
  case 'v':
    var = (Variable*)v;
    m = 4;
    c = 0;
    if (var->name[0] == '_')
      m += 2;
    if (prefix != NULL)
      c = strlen(prefix)+1;
    if (var->indextype[0] != '0') {
      j = 256;
      s = malloc(j);
    } else {
      s = malloc(strlen(var->name) + m + c + 2);
    }
    if (var->name[0] == '_')
      while (( s[i] = underscore[i] )) i++;
    else
      while (( s[i] = varprefix[i] )) i++;
    if (c) {
      while (( s[i] = prefix[i-m] )) i++;
      s[i] = '_';
      i++;
    }
    m = i;
    while (( s[i] = var->name[i-m] )) i++;
    for (m=0;var->indextype[m] != '0';m++) {
      t = NULL;
      switch (var->indextype[m]) {
      case 'v':
	SNPRINTF_REALLOC(snprintf(s+i, j-i,"_%s", (t = valueToLlvmString(var->index[m], prefix, localvars))),
			 snprintf(s+i, j-i,"_%s", t));
	break;
      case 'n':
	SNPRINTF_REALLOC(snprintf(s+i, j-i,"_%d", *(int*)var->index[m]),
			 snprintf(s+i, j-i,"_%d", *(int*)var->index[m]));
	break;
      default:
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "_%s", (char*)var->index[m]),
			 snprintf(s+i, j-i, "_%s", (char*)var->index[m]));
      }
      free(t);
    }
    break;
  case 'a':
    fd = v->data;
    j = 256;
    s = malloc(j);
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "_block_%d", fd->blocknum),
		     snprintf(s+i, j-i, "_block_%d", fd->blocknum));
    /* if (fd->arguments && fd->arguments->next) { */
    /*   n = 0; */
    /*   for (node = fd->arguments->next;node != NULL; node = node->next) { */
    /*  t = valueToLlvmString(node->data); */
    /*  k = snprintf(s+i, j-i, "pop rax\nmov [%s], rax\n", t);/\*= options.%s || options['%d'];\n", t, t, n);*\/ */
    /*  if (k+i >= j) { */
    /*    j *= 2; */
    /*    s = realloc(s, j); */
    /*    k = snprintf(s+i, j-i, "pop rax\nmov [%s], rax\n", t);/\*= options.%s || options['%d'];\n", t, t, n);*\/ */
    /*  } */
    /*  i += k; */
    /*  n++; */
    /*  free(t); */
    /*   } */
    /* } */
    /* for (node = fd->statements;node != NULL; node = node->next) { */
    /*   k = snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(node->data))); */
    /*   if (i+k >= j) { */
    /*  j *= 2; */
    /*  s = realloc(s, j); */
    /*  k = snprintf(s+i, j-i, "%s\n", t); */
    /*   } */
    /*   i += k; */
    /*   free(t); */
    /* } */
    /* snprintf(s+i, j-i, "}"); */
    break;
  case 'c':
    fv = (FuncVal*)v;
    fvname = fv->name;
    if (constants < fvname && fvname < constants+140 && (fvname-constants)%2)
      fvname--;
    j = 256;
    s = malloc(j);
    if (fvname == constants+38) {
      digit = ((Value*)fv->arglist->next->next->data)->type;
      if (digit == 'c' || digit == 'b')
	k = snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(fv->arglist->next->next->data, prefix, localvars)));
      else
	k = snprintf(s+i, j-i, "mov rax, offset %s\n", (t = valueToLlvmString(fv->arglist->next->next->data, prefix, localvars)));
      if (i+k >= j) {
	j *= 2;
	s = realloc(s, j);
	digit = ((Value*)fv->arglist->next->next->data)->type;
	if (digit == 'c')
	  k = snprintf(s+i, j-i, "%s\n", t);
	else
	  k = snprintf(s+i, j-i, "mov rax, offset %s\n", t);
      }
      i += k;
      free(t);
      node = NULL;
      if (localvars->next != NULL || localvars->data != NULL) {
	for (node = localvars;node != NULL; node = node->next) {
	  if (((Value*)node->data)->data == ((Value*)fv->arglist->next->data)->data)
	    break;
	}
      }
      t = valueToLlvmString(fv->arglist->next->data, prefix, localvars);
      if (node == NULL) {
	if (localvars != NULL)
	  addToListEnd(localvars, fv->arglist->next->data);
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, [%s]\npush rbx\n", t),
			 snprintf(s+i, j-i, "mov rbx, [%s]\npush rbx\n", t));
      } else if (((char*)((Value*)node->data)->data)[0] == '_') {
	s[0] = '\0';
	free(t);
	return s;
      }
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov [%s], rax", t),
		       snprintf(s+i, j-i, "mov [%s], rax", t));
      free(t);
    } else if (fvname == constants+128) {
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "throw %s", (t = valueToLlvmString(fv->arglist->next->data, prefix, localvars))),
		       snprintf(s+i, j-i, "throw %s", t));
      free(t);
    } else if (fvname == constants+134) {
      var = ((List*)fv->arglist->data)->next->data;
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "try {\n%s} catch (%s) {\nvar __fffll_%s = %s;\n", (t = valueToLlvmString(fv->arglist->next->data, prefix, localvars)), var->name, var->name, var->name),
		       snprintf(s+i, j-i, "try {\n%s} catch (%s) {\nvar __fffll_%s = %s;\n", t, var->name, var->name, var->name));
      free(t);
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nif (__fffll_%s == %s) throw %s;\n}\n", (t = valueToLlvmString(findInTree(((List*)fv->arglist->data)->data, var->name), prefix, localvars)), var->name, var->name, var->name),
		       snprintf(s+i, j-i, "%s\nif (__fffll_%s == %s) throw %s;\n}\n", t, var->name, var->name, var->name));
      free(t);
    } else if (fvname == constants+44) {
      c = branchnum++;
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nadd rax, 4\ncmp qword ptr [rax], 0x0\nje _branch_%d\n", (t = valueToLlvmString(fv->arglist->next->data, prefix, localvars)), c),
		       snprintf(s+i, j-i, "%s\nadd rax, 4\ncmp qword ptr [rax], 0x0\nje _branch_%d\n", t, c));
      free(t);
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s", (t = valueToLlvmString(fv->arglist->next->next->data, prefix, localvars))),
		       snprintf(s+i, j-i, "%s", t));
      free(t);
      b = -1;
      if (fv->arglist->next->next->next != NULL) {
	b = c;
	c = branchnum++;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "jmp _branch_%d\n", c),
			 snprintf(s+i, j-i, "jmp _branch_%d\n", c));
      }
      for (node = fv->arglist->next->next->next;node != NULL; node = node->next) {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "_branch_%d:\n", b),
			 snprintf(s+i, j-i, "_branch_%d:\n", b));
	if (node->next && node->next->data) {
	  if (node->next->next) {
	    b = branchnum++;
	  } else {
	    b = c;
	  }
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nadd rax, 4\ncmp qword ptr [rax], 0x0\nje _branch_%d\n", (t = valueToLlvmString(node->data, prefix, localvars)), b),
			   snprintf(s+i, j-i, "%s\nadd rax, 4\ncmp qword ptr [rax], 0x0\nje _branch_%d\n", t, b));
	  free(t);
	  node = node->next;
	}
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s", (t = valueToLlvmString(node->data, prefix, localvars))),
			 snprintf(s+i, j-i, "%s", t));
	free(t);
	if (node->next) {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "jmp _branch_%d\n", c),
			   snprintf(s+i, j-i, "jmp _branch_%d\n", c));
	}
      }
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "_branch_%d:\n", c),
		       snprintf(s+i, j-i, "_branch_%d:\n", c));
    } else if (fvname == constants+48) {
      c = -1;
      b = branchnum++;
      if (fv->arglist->data) {
	var = ((List*)fv->arglist->data)->next->data;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\npush rax\n_branch_%d:\npop rax\npush 0\npush rax\ncall _list_next\npush rax\nmov [%s], rax\n", (t = valueToLlvmString(findInTree(((List*)fv->arglist->data)->data, var->name), prefix, localvars)), b, var->name),
			 snprintf(s+i, j-i, "%s\npush rax\n_branch_%d:\npop rax\npush 0\npush rax\ncall _list_next\npush rax\nmov [%s], rax\n", t, b, var->name));
	free(t);
      } else {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "_branch_%d:\n", b),
			 snprintf(s+i, j-i, "_branch_%d:\n", b));
      }
      node = fv->arglist->next;
      t = NULL;
      if (fv->arglist->next->next) {
	c = branchnum++;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nadd rax, 4\ncmp qword ptr [rax], 0x0\nje _branch_%d\n", (t = valueToLlvmString(node->data, prefix, localvars)), c),
			 snprintf(s+i, j-i, "%s\nadd rax, 4\ncmp qword ptr [rax], 0x0\nje _branch_%d\n", t, c));
	node = fv->arglist->next->next;
      }
      free(t);
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(node->data, prefix, localvars))),
		       snprintf(s+i, j-i, "%s\n", t));
      free(t);
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "jmp _branch_%d\n", b),
		       snprintf(s+i, j-i, "jmp _branch_%d\n", b));
      if (c >= 0) {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "_branch_%d:", c),
			 snprintf(s+i, j-i, "_branch_%d:", c));
      }
    } else {
      n = i;
      b = 1;
      s[i] = '\0';
      if (fv->arglist) {
	if (fv->arglist->next) {
	  for (node = fv->arglist->next;node != NULL; node = node->next) {
	    i = n;
	    t = valueToLlvmString(node->data, prefix, localvars);
	    argpush = malloc(j-i+1);
	    argpush[j-i] = '\0';
	    k = 0;
	    while ((argpush[k] = s[i+k])) {
	      k++;
	      if (j-i == k) break;
	    }
	    digit = ((Value*)node->data)->type;
	    if (digit == 'c' || digit == 'b') {
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\npush rax\n%s", t, argpush),
			       snprintf(s+i, j-i, "%s\npush rax\n%s", t, argpush));
	    } else if (digit == 'n' || digit == 's') {
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, offset %s\npush rax\n%s", t, argpush),
			       snprintf(s+i, j-i, "mov rax, offset %s\npush rax\n%s", t, argpush));
	    } else {
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s]\npush rax\n%s", t, argpush),
			       snprintf(s+i, j-i, "mov rax, [%s]\npush rax\n%s", t, argpush));
	    }
	    free(t);
	    free(argpush);
	    b++;
	  }
	}
      }
      i = n;
      argpush = malloc(j-i+1);
      argpush[j-i] = '\0';
      k = 0;
      while ((argpush[k] = s[i+k])) {
	k++;
	if (j-i == k) break;
      }
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "push 0\n%s", argpush),
		       snprintf(s+i, j-i, "push 0\n%s", argpush));
      free(argpush);
      if (constants < fvname && fvname < constants+140) {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "call %s\n", fvname),
			 snprintf(s+i, j-i, "call %s\n", fvname));
	t = NULL;
      } else {
	t = valueToLlvmString(fv->val, prefix, localvars);
	if (fv->name == anonfunc) {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "call %s\n", t),
			   snprintf(s+i, j-i, "call %s\n", t));
	} else if (fv->val->type == 'c') {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\ncall rax\n", t),
			   snprintf(s+i, j-i, "%s\ncall rax\n", t));
	} else {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "call [%s]\n", t),
			   snprintf(s+i, j-i, "call [%s]\n", t));
	}
      }
      free(t);
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "add rsp, %d\n", 8*b),
		       snprintf(s+i, j-i, "add rsp, %d\n", 8*b));
      i--;
      s[i] = '\0';
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
    s = malloc(9+j);
    j += 8;
    i = 0;
    while ((s[i] = strptr[i])) i++;
    s[j--] = '\0';
    while (i != j) {
      s[j--] = k%10 + 48;
      k /= 10;
    }
    s[j--] = '_';
    break;
  case 'b':
    be = (BoolExpr*)v;
    j = 256;
    s = malloc(j);
    for (node = be->stack;node != NULL; node = node->next) {
      BOOL_REALLOC(node->data);
      if (node->next && node->next->next) {
	node = node->next;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "push rax\n"),
			 snprintf(s+i, j-i, "push rax\n"));
	BOOL_REALLOC(node->next->data);
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\nxchg rax, rbx\n"),
			 snprintf(s+i, j-i, "pop rbx\nxchg rax, rbx\n"));
	switch (*(char*)(node->data)) {
	case '&':
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "test rax, rax\nsetnz al\nand rax, 0xFF\ntest rbx, rbx\nsetnz bl\nand rbx, 0xFF\ntest al, bl\nsetnz al\nand rax, 0xFF\n"),
			   snprintf(s+i, j-i, "test rax, rax\nsetnz al\nand rax, 0xFF\ntest rbx, rbx\nsetnz bl\nand rbx, 0xFF\ntest al, bl\nsetnz al\nand rax, 0xFF\n"));
	  break;
	case '|':
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "or rax, rbx\nsetnz al\nand rax, 0xFF\n"),
			   snprintf(s+i, j-i, "or rax, rbx\nsetnz al\nand rax, 0xFF\n"));
	  break;
	case '>':
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp rax, rbx\nsetg al\nand rax, 0xFF\n"),
			   snprintf(s+i, j-i, "cmp rax, rbx\nsetg al\nand rax, 0xFF\n"));
	  break;
	case '<':
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp rax, rbx\nsetl al\nand rax, 0xFF\n"),
			   snprintf(s+i, j-i, "cmp rax, rbx\nsetl al\nand rax, 0xFF\n"));
	  break;
	case '=':
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp rax, rbx\nsete al\nand rax, 0xFF\n"), 
			   snprintf(s+i, j-i, "cmp rax, rbx\nsete al\nand rax, 0xFF\n"));
	  break;
	case '?': /* oh fuck. */
	  break;
	case '~': /* oh fuck */
	  break;
	}
	node = node->next;
      }
    }
    if (be->neg) {
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "xor rax, 1\n"),
		       snprintf(s+i, j-i, "xor rax, 1\n"));
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rdx, rax\ncall _allocvar\nmov byte ptr [rax], 'b'\nadd rax, 4\nmov qword ptr [rax], rdx\nsub rax, 4"),
		     snprintf(s+i, j-i, "mov rdx, rax\ncall _allocvar\nmov byte ptr [rax], 'b'\nadd rax, 4\nmov qword ptr [rax], rdx\nsub rax, 4"));
    break;
  case 'd':
    j = 256;
    s = malloc(j);
    for (node = v->data; node != NULL; node = node->next) {
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(node->data, prefix, localvars))),
		       snprintf(s+i, j-i, "%s\n", t));
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
	k = snprintf(s+i, j-i, "%s: %s, ", var->name, (t = valueToLlvmString(findInTree(((List*)((List*)v->data)->data)->data, var->name), prefix, localvars)));
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
	k = snprintf(s+i, j-i, "%d: %s, ", m++, (t = valueToLlvmString(node->data, prefix, localvars)));
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
    k = snprintf(s+i, j-i, "0x0 # Range");
    i += k;
    break;
  case 'n':
    for (node = numlist;node != NULL;node = node->next) {
      if (*(double*)v->data == *(double*)node->data)
	break;
      i++;
    }
    j = 1;
    k = i;
    while (i > 9) {
      i /= 10;
      j++;
    }
    s = malloc(9+j);
    j += 8;
    i = 0;
    while ((s[i] = numptr[i])) i++;
    s[j--] = '\0';
    while (i != j) {
      s[j--] = k%10 + 48;
      k /= 10;
    }
    s[j--] = '_';
    break;
  default:
    s = valueToString(v);
    break;
  }
  return s;
}

char* doubleToHexString(double* d) {
  int i = 0, j, b, c;
  char* s, *t, digit;
  j = 24;
  s = malloc(j);
  t = (char*)d;
  i += snprintf(s+i, j-i, "0x");
  c = 1;
  for (b=5;c || b != 5;b++) {
    if (b == 9) {
      i += snprintf(s+i, j-i, ", 0x");
      b = 1;
      c = 0;
    }
    digit = (t[8-b]/16)+48;
    if (digit < 48) digit += 16;
    if (57 < digit)
      i += snprintf(s+i, j-i, "%c", digit+39);
    else
      i += snprintf(s+i, j-i, "%c", digit);
    digit = (t[8-b]%16)+48;
    if (digit < 48) digit += 16;
    if (57 < digit)
      i += snprintf(s+i, j-i, "%c", digit+39);
    else
      i += snprintf(s+i, j-i, "%c", digit);
  }
  return s;
}
