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

#include <curl/curl.h>
#include "list.h"

#ifndef FFFLLVALUE
#define FFFLLVALUE

typedef struct value Value;
struct value {
  char type;
  int refcount;
  void* data;
};

typedef struct boolval BoolExpr;
struct boolval {
  char type;
  int refcount;
  List* stack;
  int neg;
  int lasteval;
};

typedef struct funcdef FuncDef;
struct funcdef {
  Value* (*evaluate)(FuncDef*, List*);
  List* statements;
  List* arguments;
  int alloc;
  int blocknum;
};

typedef struct funcval FuncVal;
struct funcval {
  char type;
  int refcount;
  Value* val;
  List* arglist;
  char* name;
  int lineno;
};

typedef struct httpval HttpVal;
struct httpval {
  char type;
  int refcount;
  CURL* curl;
  char* url;
  char* buf;
  int pos;
  int bufsize;
};

typedef struct stringval String;
struct stringval {
  int refcount;
  char* val;
};

typedef struct rangelist Range;
struct rangelist {
  char type;
  int refcount;
  Value* computed;
  Value* start;
  Value* increment;
  Value* end;
};

typedef struct variable Variable;
struct variable {
  char type;
  int refcount;
  char* name;
  char* indextype;
  void** index;
};

#endif

String* addToStringList(String* s);
int cleanupFffll(Value* v);
int errmsg(char* err);
int errmsgf(char* format, char* s);
Value* evaluateStatements(List* sl);
Value* evaluateValue(Value* v);
int freeBoolExpr(BoolExpr* be);
int freeRange(Range* range);
int freeValue(Value* val);
int freeValueList(List* r);
int freeVariable(Variable* var);
BoolExpr* newBoolExpr(Value* val);
FuncDef* newBuiltinFuncDef(Value* (*evaluate)(FuncDef*, List*), int alloc);
FuncDef* newFuncDef(List* al, List* sl, int alloc);
FuncVal* newFuncVal(Value* val, List* arglist, char* name, int ln);
HttpVal* newHttpVal(char* url);
String* newString(char* s);
Range* newRange(Value* start, Value* end, Value* increment);
Value* newValue(char type, void* data);
Variable* newVariable(char* name);
Variable* parseVariable(char* name);
