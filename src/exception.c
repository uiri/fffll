#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"

extern List* jmplist;

char* diemsg;

char* errmsglist[] = {
  "Unknown error :(",

  "Wrong number of arguments for cat",
  "Wrong number of arguments for for",
  "Wrong number of arguments for head",
  "Wrong number of arguments for if",
  "Wrong number of arguments for len",
  "Wrong number of arguments for open",
  "Wrong number of arguments for push",
  "Wrong number of arguments for rcp",
  "Wrong number of arguments for read",
  "Wrong number of arguments for save",
  "Wrong number of arguments for set",
  "Wrong number of arguments for tail",
  "Wrong number of arguments for tok",
  "Wrong number of arguments for write",

  /* 15 */
  "Expected List as argument for head",
  "Expected List, String or Statementlist as argument for len",
  "Expected List as argument for push",
  "Expected I/O stream as argument for read",
  "Expected Name as argument for set",
  "Expected List as argument for tail",
  "Expected String as argument for tok",
  "Expected I/O stream as argument for write",

  /* 23 */
  "Failed to read from I/O Stream",
  "Attempt to read past the end of I/O Stream",
  "Only List can be indexed",
  "List index out of bounds",
  "Key not found",
  "Illegal attempt to modify a range"
};

Value* recoverErr() {
  if (jmplist && jmplist->data) {
    longjmp(*((jmp_buf*)jmplist->data), 1);
  } else {
    errmsgf("%s", diemsg);
    return NULL;
  }
}

void makeErrMsg(int i) {
  int l;
  l = strlen(errmsglist[i]);
  diemsg = malloc(l+1);
  strcpy(diemsg, errmsglist[i]);
}

Value* raiseErr(int i) {
  makeErrMsg(i);
  return recoverErr();
}
