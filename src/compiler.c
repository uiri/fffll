#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compiler.h"
#include "regex.h"
#include "tree.h"

extern char* constants;
extern char* anonfunc;
extern List* globalvarlist;
extern List* stringlist;
extern List* numlist;
extern List* constlist;
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
  switch (digit) {\
  case 'c':\
  case 'b':\
  case 'x':\
  case 'l':\
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nmov rax, [rax+4]\n", (t = valueToLlvmString(data, prefix, localvars))),\
		     snprintf(s+i, j-i, "%s\nmov rax, [rax+4]\n", t));\
    break;\
  case 'n':\
  case 's':\
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s+4]\n", (t = valueToLlvmString(data, prefix, localvars))),\
		     snprintf(s+i, j-i, "mov rax, [%s+4]\n", t));\
    break;\
  default:\
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s]\nmov rax, [rax+4]\n", (t = valueToLlvmString(data, prefix, localvars))),\
		     snprintf(s+i, j-i, "mov rax, [%s]\nmov rax, [rax+4]\n", t));\
    break;\
  }\
  free(t)

int branchnum = 0;
int blocknum = -1;
int regexnum = 0;

void printFunc(List* arglist, List* statementlist) {
  List* node, *localvarlist;
  char* s, *prefix, *prepend, *t;
  int i, j, k, b;
  Value* v, *funcVal;
  blocknum++;
  printf("_block_%d:\npush rbp\nmov rbp, rsp\n", blocknum);
  localvarlist = newList();
  prefix = NULL;
  if (buffernames)
    prefix = buffernames->data;
  i = 16;
  if (arglist) {
    for (node = arglist->next; node != NULL; node = node->next) {
      if (node->data) {
	t = valueToLlvmString(node->data, prefix, localvarlist);
	printf("mov rdx, [%s]\npush rdx\ntest byte ptr [%s+8], 0x80\njnz _block_%d_%d\nmov rdx, [rbp+%d]\nmov [%s], rdx\njmp _block_%d_%d\n_block_%d_%d:\nsub byte ptr [%s+8], 0x80\n_block_%d_%d:\n", t, t, blocknum, i, i, t, blocknum, i+1, blocknum, i, t, blocknum, i+1);
	i += 8;
	free(t);
      }
    }
    b = i;
    if (arglist->data) {
      node = arglist->data;
      for (node = node->next; node != NULL; node = node->next) {
	prepend = valueToLlvmString(node->data, prefix, localvarlist);
	v = findInTree(((List*)arglist->data)->data, ((Variable*)node->data)->name);
	t = valueToLlvmString(v, prefix, NULL);
	printf("mov rdx, [%s]\npush rdx\n", prepend);
	switch (v->type) {
	case 'c':
	case 'b':
	  printf("%s\nmov [%s], rax\n", t, prepend);
	  break;
	case 'n':
	case 's':
	  printf("mov rax, offset %s\nmov [%s], rax\n", t, prepend);
	  break;
	default:
	  printf("mov rax, [%s]\nmov [%s], rax\n", t, prepend);
	  break;
	}
	free(t);
	free(prepend);
	prepend = NULL;
      }
      node = arglist->data;
      for (node = node->next; node != NULL; node = node->next) {
	t = valueToLlvmString(node->data, prefix, localvarlist);
	if (((Value*)node->data)->type == 'v' && ((Variable*)node->data)->name[0] == '_') {
	  free(t);
	  continue;
	}
	printf("mov rdx, [rbp+%d]\ntest rdx, rdx\njz _block_%d_default\nmov [%s], rdx\n", i, blocknum, t);
	i += 8;
	free(t);
      }
      printf("_block_%d_default:\n", blocknum);
    }
  }
  funcVal = newValue('d', statementlist);
  prepend = valueToLlvmString(funcVal, prefix, localvarlist);
  free(funcVal);
  i = 0;
  j = 256;
  s = malloc(j);
  s[0] = '\0';
  if (localvarlist->next != NULL || localvarlist->data != NULL) {
    for (node = localvarlist; node != NULL; node = node->next) {
      t = valueToLlvmString(node->data, prefix, NULL);
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, [%s]\npush rbx\n", t),
		       snprintf(s+i, j-i, "mov rbx, [%s]\npush rbx\n", t));
      free(t);
    }
  }
  SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s", prepend),
		   snprintf(s+i, j-i, "%s", prepend));
  free(prepend);
  prepend = NULL;
  if (localvarlist->next != NULL || localvarlist->data != NULL) {
    for (node = localvarlist; node != NULL; node = node->next) {
      prepend = malloc(i+1);
      k = 0;
      while (( prepend[k] = s[k] )) k++;
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\nmov [%s], rbx\n%s", (t = valueToLlvmString(node->data, prefix, NULL)), prepend),
		       snprintf(s+i, j-i, "pop rbx\nmov [%s], rbx\n%s", t, prepend));
      free(t);
      free(prepend);
    }
  }
  b = 0;
  if (arglist) {
    if (arglist->data) {
      node = arglist->data;
      for (node = node->next; node != NULL; node = node->next) {
	prepend = malloc(b+1);
	k = 0;
	while (( prepend[k] = s[i+k] )) k++;
	t = valueToLlvmString(node->data, prefix, NULL);
	if (((Value*)node->data)->type == 'v' && (((Variable*)node->data)->name)[0] == '_') {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rdx\n%s", prepend),
			   snprintf(s+i, j-i, "pop rdx\n%s", prepend));
	} else {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rdx\nmov [%s], rdx\n%s", t, prepend),
			   snprintf(s+i, j-i, "pop rdx\nmov [%s], rdx\n%s", t, prepend));
	}
	free(t);
	b += k;
	free(prepend);
      }
    }
    for (node = arglist->next; node != NULL; node = node->next) {
      if (node->data) {
	prepend = malloc(b+1);
	k = 0;
	while (( prepend[k] = s[i+k] )) k++;
	t = valueToLlvmString(node->data, prefix, NULL);
	if (((Value*)node->data)->type == 'v' && (((Variable*)node->data)->name)[0] == '_') {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rdx\n%s", prepend),
			   snprintf(s+i, j-i, "pop rdx\n%s", prepend));
	} else {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rdx\nmov [%s], rdx\n%s", t, prepend),
			   snprintf(s+i, j-i, "pop rdx\nmov [%s], rdx\n%s", t, prepend));
	}
	free(t);
	b += k;
	free(prepend);
      }
    }
  }
  printf("%s", s);
  free(s);
  freeList(localvarlist);
  printf("mov rsp, rbp\npop rbp\nret\n");
}

char* valueToLlvmString(Value* v, char* prefix, List* localvars) {
  char* s, *t, *argpush, *fvname, digit;
  char* strptr = "strlist", *numptr = "numlist", *underscore = "const_", *varprefix = "var_";
  Variable* var;
  GlobalVar* gvar;
  FuncDef* fd;
  FuncVal* fv;
  BoolExpr* be;
  RegexState* restate;
  Range* r;
  Value* u;
  List* node, *submatches, *globalnode;
  int i = 0, j, k, m, n, b, c, andor;
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
    for (k = 0;var->indextype[k] != '0';k++);
    if ((!k && var->name[0] == '_') || (var->indextype[k-1] == 'u' && *((char*)(var->index[k-1])) == '_'))
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
    for (globalnode = globalvarlist; globalnode != NULL; globalnode = globalnode->next) {
      if (!globalnode->data) continue;
      if (!strcmp(((GlobalVar*)globalnode->data)->val, s))
	break;
    }
    if (globalnode == NULL) {
      gvar = malloc(sizeof(GlobalVar));
      gvar->var = var->name;
      t = malloc(j);
      k = 0;
      while (( t[k] = s[k] )) k++;
      gvar->val = t;
      t = NULL;
      addToListEnd(globalvarlist, gvar);
    }
    break;
  case 'a':
    fd = v->data;
    j = 256;
    s = malloc(j);
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "_block_%d", fd->blocknum),
		     snprintf(s+i, j-i, "_block_%d", fd->blocknum));
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
      if (digit == 'c' || digit == 'b' || digit == 'l') {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\n", (t = valueToLlvmString(fv->arglist->next->next->data, prefix, localvars))),
			 snprintf(s+i, j-i, "%s\n", t));
      } else {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, offset %s\n", (t = valueToLlvmString(fv->arglist->next->next->data, prefix, localvars))),
			 snprintf(s+i, j-i, "mov rax, offset %s\n", t));
      }
      free(t);
      node = NULL;
      if (((char*)((Value*)fv->arglist->next->data)->data)[0] == '_') {
	if (constlist->next != NULL || constlist->data != NULL) {
	  for (node = constlist;node != NULL; node = node->next) {
	    if (((Value*)node->data)->data == ((Value*)fv->arglist->next->data)->data)
	      break;
	  }
	}
	t = valueToLlvmString(fv->arglist->next->data, prefix, localvars);
	if (node == NULL && constlist != NULL) {
	  addToListEnd(constlist, fv->arglist->next->data);
	} else {
	  s[0] = '\0';
	  free(t);
	  return s;
	}
      } else {
	if (localvars && (localvars->next != NULL || localvars->data != NULL)) {
	  for (node = localvars;node != NULL; node = node->next) {
	    if (((Value*)node->data)->data == ((Value*)fv->arglist->next->data)->data)
	      break;
	  }
	  if (node == NULL) {
	    addToListEnd(localvars, fv->arglist->next->data);
	  }
	}
	t = valueToLlvmString(fv->arglist->next->data, prefix, localvars);
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
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\ncmp qword ptr [rax+4], 0x0\nje _branch_%d\n", (t = valueToLlvmString(fv->arglist->next->data, prefix, localvars)), c),
		       snprintf(s+i, j-i, "%s\ncmp qword ptr [rax+4], 0x0\nje _branch_%d\n", t, c));
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
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\ncmp qword ptr [rax+4], 0x0\nje _branch_%d\n", (t = valueToLlvmString(node->data, prefix, localvars)), b),
			   snprintf(s+i, j-i, "%s\ncmp qword ptr [rax+4], 0x0\nje _branch_%d\n", t, b));
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
      b = branchnum++;
      c = branchnum++;
      if (fv->arglist->data) {
	var = ((List*)fv->arglist->data)->next->data;
	u = findInTree(((List*)fv->arglist->data)->data, var->name);
	BOOL_REALLOC(u);
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "push rax\n_branch_%d:\npop rax\ncall _list_next\ntest rax, rax\njz _branch_%d\nmov [%s], rax\npush rbx\n", b, c, (t = valueToLlvmString(((Value*)var), prefix, localvars))),
			 snprintf(s+i, j-i, "push rax\n_branch_%d:\npop rax\ncall _list_next\ntest rax, rax\njz _branch_%d\nmov [%s], rax\npush rbx\n", b, c, t));
	free(t);
      } else {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "_branch_%d:\n", b),
			 snprintf(s+i, j-i, "_branch_%d:\n", b));
      }
      node = fv->arglist->next;
      t = NULL;
      if (fv->arglist->next->next) {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\ncmp qword ptr [rax+4], 0x0\nje _branch_%d\n", (t = valueToLlvmString(node->data, prefix, localvars)), c),
			 snprintf(s+i, j-i, "%s\ncmp qword ptr [rax+4], 0x0\nje _branch_%d\n", t, c));
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
	if (fv->arglist->data) {
	  node = fv->arglist->data;
	  c = 0;
	  for (node = node->next; node != NULL; node = node->next) {
	    var = node->data;
	    argpush = valueToLlvmString(node->data, prefix, localvars);
	    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s]\npush rax\n", argpush),
			     snprintf(s+i, j-i, "mov rax, [%s]\npush rax\n", argpush));
	    if (var->name[0] == '_') {
	      c = branchnum++;
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "test rax, rax\njne _branch_%d\n", c),
			       snprintf(s+i, j-i, "test rax, rax\njne _branch_%d\n", c));
	    }
	    u = findInTree(((List*)fv->arglist->data)->data, var->name);
	    t = valueToLlvmString(u, prefix, localvars);
	    switch (u->type) {
	    case 'c':
	    case 'b':
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\nmov [%s], rax\n", t, argpush),
			       snprintf(s+i, j-i, "%s\nmov [%s], rax\n", t, argpush));
	      break;
	    case 'n':
	    case 's':
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, offset %s\nmov [%s], rax\n", t, argpush),
			       snprintf(s+i, j-i, "mov rax, offset %s\nmov [%s], rax\n", t, argpush));
	      break;
	    default:
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s]\nmov [%s], rax\n", t, argpush),
			       snprintf(s+i, j-i, "mov rax, [%s]\nmov [%s], rax\n", t, argpush));
	      break;
	    }
	    SNPRINTF_REALLOC(snprintf(s+i, j-i, "add byte ptr [%s+8], 0x80\n", argpush),
			     snprintf(s+i, j-i, "add byte ptr [%s+8], 0x80\n", argpush));
	    free(t);
	    free(argpush);
	    if (var->name[0] == '_') {
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "_branch_%d:\n", c),
			       snprintf(s+i, j-i, "_branch_%d:\n", c));
	    }
	  }
	  n = i;
	  s[i] = '\0';
	}
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
	    switch (digit) {
	    case 'c':
	    case 'b':
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "%s\npush rax\n%s", t, argpush),
			       snprintf(s+i, j-i, "%s\npush rax\n%s", t, argpush));
	      break;
	    case 'n':
	    case 's':
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, offset %s\npush rax\n%s", t, argpush),
			       snprintf(s+i, j-i, "mov rax, offset %s\npush rax\n%s", t, argpush));
	      break;
	    default:
	      SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, [%s]\npush rax\n%s", t, argpush),
			       snprintf(s+i, j-i, "mov rax, [%s]\npush rax\n%s", t, argpush));
	      break;
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
      n = i;
      if (fv->arglist && fv->arglist->data) {
	node = fv->arglist->data;
	for (node = node->next;node != NULL; node = node->next) {
	  i = n;
	  t = valueToLlvmString(node->data, prefix, localvars);
	  argpush = malloc(j-i+1);
	  argpush[j-i] = '\0';
	  k = 0;
	  while ((argpush[k] = s[i+k])) {
	    k++;
	    if (j-i == k) break;
	  }
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\n"),
			   snprintf(s+i, j-i, "pop rbx\n"));
	  if (((Value*)node->data)->type == 'v' && ((Variable*)node->data)->name[0] == '_') {
	    c = branchnum++;
	    SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp rbx, [%s]\nje _branch_%d\nmov [%s], rbx\n_branch_%d:\n%s", t, c, t, c, argpush),
			     snprintf(s+i, j-i, "cmp rbx, [%s]\nje _branch_%d\nmov [%s], rbx\n_branch_%d:\n%s", t, c, t, c, argpush));
	  } else {
	    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov [%s], rbx\n%s", t, argpush),
			     snprintf(s+i, j-i, "mov [%s], rbx\n%s", t, argpush));
	  }
	  free(t);
	  free(argpush);
	}
      }
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
    andor = 0;
    if (be->stack->data) {
      BOOL_REALLOC(be->stack->data);
    }
    for (node = be->stack->next;node != NULL; node = node->next) {
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "push rax\n"),
		       snprintf(s+i, j-i, "push rax\n"));
      switch (*(char*)(node->data)) {
      case '&':
	andor = 1;
	node = node->next;
	BOOL_REALLOC(node->data);
	break;
      case '|':
	andor = 2;
	node = node->next;
	BOOL_REALLOC(node->data);
	break;
      default:
	if (node->next) {
	  if (andor) andor += 2;
	  BOOL_REALLOC(node->next->data);
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\nxchg rax, rbx\n"),
			   snprintf(s+i, j-i, "pop rbx\nxchg rax, rbx\n"));
	  switch (*(char*)(node->data)) {
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
	  case '~':
	    SNPRINTF_REALLOC(snprintf(s+i, j-i, "test rbx, rbx\nsetnz al\n"),
			     snprintf(s+i, j-i, "test rbx, rbx\nsetnz al\n"));
	    break;
	  }
	  node = node->next;
	  break;
	}
      }
      switch (andor) {
      case 4:
	andor = 0;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\nor rax, rbx\nsetnz al\nand rax, 0xFF\n"),
			 snprintf(s+i, j-i, "pop rbx\nor rax, rbx\nsetnz al\nand rax, 0xFF\n"));
	break;
      case 3:
	andor = 0;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\ntest rax, rax\nsetnz al\nand rax, 0xFF\ntest rbx, rbx\nsetnz bl\nand rbx, 0xFF\ntest al, bl\nsetnz al\nand rax, 0xFF\n"),
			 snprintf(s+i, j-i, "pop rbx\ntest rax, rax\nsetnz al\nand rax, 0xFF\ntest rbx, rbx\nsetnz bl\nand rbx, 0xFF\ntest al, bl\nsetnz al\nand rax, 0xFF\n"));
	break;
      }
    }
    if (andor) {
      switch (andor) {
      case 2:
	andor = 0;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\nor rax, rbx\nsetnz al\nand rax, 0xFF\n"),
			 snprintf(s+i, j-i, "pop rbx\nor rax, rbx\nsetnz al\nand rax, 0xFF\n"));
	break;
      case 1:
	andor = 0;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rbx\ntest rax, rax\nsetnz al\nand rax, 0xFF\ntest rbx, rbx\nsetnz bl\nand rbx, 0xFF\ntest al, bl\nsetnz al\nand rax, 0xFF\n"),
			 snprintf(s+i, j-i, "pop rbx\ntest rax, rax\nsetnz al\nand rax, 0xFF\ntest rbx, rbx\nsetnz bl\nand rbx, 0xFF\ntest al, bl\nsetnz al\nand rax, 0xFF\n"));
	break;
      }
    }
    if (be->neg) {
      SNPRINTF_REALLOC(snprintf(s+i, j-i, "xor rax, 1\n"),
		       snprintf(s+i, j-i, "xor rax, 1\n"));
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rdx, rax\ncall _allocvar\nmov byte ptr [rax], 'b'\nmov qword ptr [rax+4], rdx"),
		     snprintf(s+i, j-i, "mov rdx, rax\ncall _allocvar\nmov byte ptr [rax], 'b'\nmov qword ptr [rax+4], rdx"));
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
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "call _alloc_list\npush rax\n"),
		     snprintf(s+i, j-i, "call _alloc_list\npush rax\n"));
    if (v->data) {
      for (node = ((List*)v->data)->next; node != NULL; node = node->next) {
	switch (((Value*)node->data)->type) {
	case 'n':
	case 's':
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, rax\ncall _alloc_list\npush rax\nmov rax, offset %s\nmov [rbx], rax\nadd rbx, 8\npop rax\nmov [rbx], rax\n", (t = valueToLlvmString(node->data, prefix, localvars))),
			   snprintf(s+i, j-i, "mov rbx, rax\ncall _alloc_list\npush rax\nmov rax, offset %s\nmov [rbx], rax\nadd rbx, 8\npop rax\nmov [rbx], rax\n", t));
	  break;
	case 'r':
	default:
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, rax\ncall _alloc_list\npush rax\npush rbx\n%s\npop rbx\nmov [rbx], rax\nadd rbx, 8\npop rax\nmov [rbx], rax\n", (t = valueToLlvmString(node->data, prefix, localvars))),
			   snprintf(s+i, j-i, "mov rbx, rax\ncall _alloc_list\npush rax\npush rbx\n%s\npop rbx\nmov [rbx], rax\nadd rbx, 8\npop rax\nmov [rbx], rax\n", t));
	  break;
	}
	free(t);
      }
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rdx\ncall _allocvar\nmov byte ptr [rax], 'l'\nmov qword ptr [rax+4], rdx"),
		     snprintf(s+i, j-i, "pop rdx\ncall _allocvar\nmov byte ptr [rax], 'l'\nmov qword ptr [rax+4], rdx"));
    break;
  case 'r':
    r = (Range*)v;
    j = 256;
    s = malloc(j);
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "call _allocstr\n"),
		     snprintf(s+i, j-i, "call _allocstr\n"));
    if (r->computed && r->computed->type != '0') {
      t = valueToLlvmString(r->computed, prefix, NULL);
    } else {
      t = "range_nan-4";
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax], rbx\n", t),
		     snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax], rbx\n", t));
    if (r->computed && r->computed->type != '0')
      free(t);
    if (r->start && r->start->type != '0') {
      t = valueToLlvmString(r->start, prefix, NULL);
    } else {
      t = "zero-4";
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax+8], rbx\n", t),
		     snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax+8], rbx\n", t));
    if (r->start && r->start->type != '0')
      free(t);
    if (r->increment && r->increment->type != '0') {
      t = valueToLlvmString(r->increment, prefix, NULL);
    } else {
      t = "one-4";
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax+16], rbx\n", t),
		     snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax+16], rbx\n", t));
    if (r->increment && r->increment->type != '0')
      free(t);
    if (r->end && r->end->type != '0') {
      t = valueToLlvmString(r->end, prefix, NULL);
    } else {
      t = "zero-4";
    }
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax+24], rbx\n", t),
		     snprintf(s+i, j-i, "mov rbx, [%s+4]\nmov [rax+24], rbx\n", t));
    if (r->end && r->end->type != '0')
      free(t);
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "push rax\ncall _allocvar\npop rbx\nmov dword ptr [rax], 0x72\nmov [rax+4], rbx\n"),
		     snprintf(s+i, j-i, "push rax\ncall _allocvar\npop rbx\nmov dword ptr [rax], 0x72\nmov [rax+4], rbx\n"));
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
  case 'x':
    j = 256;
    s = malloc(j);
    t = ((String*)v->data)->val;
    restate = newRegex(t);
    b = regexnum++;
    regexnum++;
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "pop rax\n_regex_%d:\ncmp byte ptr [rax], 0\nje _regex_%d\n", b, b+1),
		     snprintf(s+i, j-i, "_regex_%d:\npop rax\ninc rax\npush rax\ndec rax\ncmp byte ptr [rax], 0\nje _regex_%d\n", b, b+1));
    submatches = newList();
    andor = 0;
    for (m = 0;t[m];m++) {
      if (andor && t[m] == '\\') {
	andor = !andor;
      } else if (t[m+1] == '?') {
	c = regexnum++;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\n_regex_%d:\n", t[m], c, c),
			 snprintf(s+i, j-i, "cmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\n_regex_%d:\n", t[m], c, c));
	m++;
      } else if (t[m+1] == '+' || t[m+1] == '*') {
	if (t[m+1] == '+') {
	  SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\n", t[m], b),
			   snprintf(s+i, j-i, "cmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\n", t[m], b));
	}
	c = regexnum++;
	n = regexnum++;
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "_regex_%d:\ncmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\njmp _regex_%d\n_regex_%d:\n", t[m], c, n, c, n),
			 snprintf(s+i, j-i, "_regex_%d:\ncmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\njmp _regex_%d\n_regex_%d:\n", t[m], c, n, c, n));
	m++;
      } else {
	SNPRINTF_REALLOC(snprintf(s+i, j-i, "cmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\n", t[m], b),
			 snprintf(s+i, j-i, "cmp byte ptr [rax], '%c'\njne _regex_%d\ninc rax\n", t[m], b));
      }
    }
    freeList(submatches);
    c = regexnum++;
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "mov rax, 1\njmp _regex_%d\n_regex_%d:\nmov rax, 0\n_regex_%d:\npush 0\n", c, b+1, c),
		     snprintf(s+i, j-i, "mov rax, 1\njmp _regex_%d\n_regex_%d:\nmov rax, 0\n_regex_%d:\npush 0\n", c, b+1, c));
    freeRegex(restate);
    break;
  default:
    s = valueToString(v);
    break;
  }
  return s;
}

char* stringToHexString(char* t) {
  char* s;
  int i, j, k, h;
  unsigned char digit;
  i = 0;
  j = 256;
  s = malloc(j);
  for (h = 0;t[h];h++) {
    digit = t[h];
    SNPRINTF_REALLOC(snprintf(s+i, j-i, "0x%02x,", digit),
		     snprintf(s+i, j-i, "0x%02x,", digit));
  }
  SNPRINTF_REALLOC(snprintf(s+i, j-i, "0x00"),
		   snprintf(s+i, j-i, "0x00"));
  return s;
}

char* doubleToHexString(double* d) {
  int i = 0, j;
  char* s, *t;
  j = 24;
  s = malloc(j);
  t = (char*)d;
  i += snprintf(s+i, j-i, "0x%x, 0x%x", *(unsigned int*)t, *(unsigned int*)(t+4));
  return s;
}
