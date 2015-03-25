#include "repl.h"

typedef struct globalvar GlobalVar;
struct globalvar {
  char* var;
  char* val;
};

void printFunc(List* argglist, List* statementlist);
char* valueToLlvmString(Value* v, char* prefix, List* localvars);
char* stringToHexString(char* t);
char* doubleToHexString(double* d);
