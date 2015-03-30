#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "regex.h"

RegexState* newRegexSub(char* regex, int k) {
  RegexState* restate, *re;
  int parencount, substart, i, l;
  char c;
  List* expr;
  restate = malloc(sizeof(RegexState));
  restate->index = k;
  restate->quant = '\0';
  l = strlen(regex);
  if (l == 1) {
    restate->c = regex[0];
    restate->expr = NULL;
    return restate;
  }
  restate->c = '\0';
  expr = newList();
  restate->expr = expr;
  parencount = 0;
  substart = -1;
  for (i = 0;i<l;i++) {
    k++;
    switch (regex[i]) {
    case '(':
      if (!parencount)
	substart = i+1;
      parencount += 1;
      break;
    case ')':
      parencount -= 1;
      if (!parencount) {
	c = regex[i];
	regex[i] = '\0';
	addToListEnd(expr->data, newRegexSub(regex+substart, k));
	regex[i] = c;
      }
    default:
      if (parencount)
	break;
      switch (regex[i]) {
      case '|':
	addToListEnd(expr, newList());
	if (expr->next)
	  expr = expr->next;
	break;
      case '?':
      case '*':
      case '+':
	re = dataInListEnd(expr->data);
	re->quant = *(regex+i);
	break;
      default:
	c = regex[i+1];
	regex[i+1] = '\0';
	addToListEnd(expr->data, newRegexSub(regex+i, k));
	regex[i+1] = c;
	break;
      }
      break;
    }
  }
  return restate;
}

RegexState* newRegex(char* regex) {
  return newRegexSub(regex, 0);
}
