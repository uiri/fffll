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

#include <curl/curl.h>
#include "list.h"

#ifndef FFFLLVALUE
#define FFFLLVALUE

typedef struct value Value;
struct value {
  int refcount;
  void* data;
  char type;
};

typedef struct boolval BoolExpr;
struct boolval {
  int refcount;
  List* stack;
  char type;
  int neg;
  int lasteval;
};

typedef struct funcdef FuncDef;
struct funcdef {
  char* name;
  Value* (*evaluate)(FuncDef*, List*);
  List* statements;
  List* arguments;
  int alloc;
};

typedef struct funcval FuncVal;
struct funcval {
  int refcount;
  char* name;
  char type;
  List* arglist;
  int lineno;
};

typedef struct httpval HttpVal;
struct httpval {
  int refcount;
  CURL* curl;
  char type;
  char* url;
  char* buf;
  int pos;
  int bufsize;
};

typedef struct itemval Item;
struct itemval {
  int refcount;
  List* data;
  char type;
  char* parent;
  char* name;
  int indir;
};

typedef struct stringval String;
struct stringval {
  int refcount;
  char* val;
};

typedef struct variable Variable;
struct variable {
  int refcount;
  char* name;
  char type;
};

#endif

String* addToStringList(String* s);
int errmsg(char* err);
int errmsgf(char* format, char* s);
int errmsgfd(char* format, char* s, int i);
Value* evaluateStatements(List* sl);
int freeBoolExpr(BoolExpr* be);
int freeFuncVal(FuncVal* fv);
int freeHttpVal(HttpVal* hv);
int freeItem(Item* item);
int freeString(String* s);
int freeValue(Value* val);
int freeValueList(List* r);
int freeVariable(Variable* var);
FuncDef* getFunction(char* name);
int hashName(char* name);
int insertFunction(FuncDef* fd);
BoolExpr* newBoolExpr(Value* val);
int newBuiltinFuncDef(char* name, Value* (*evaluate)(FuncDef*, List*), int alloc);
FuncDef* newFuncDef(char* name, List* al, List* sl, int alloc);
FuncVal* newFuncVal(char* name, List* arglist, int ln);
HttpVal* newHttpVal(char* url);
Item* newItem(char* name, char* parent, int j);
String* newString(char* s);
Value* newValue(char type, void* data);
Variable* newVariable(char* name);
Value* valueFromName(char* name);
