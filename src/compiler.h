#include "repl.h"

void printFunc(List* argglist, List* statementlist);
char* valueToLlvmString(Value* v, char* prefix, List* localvars);
char* doubleToHexString(double* d);
