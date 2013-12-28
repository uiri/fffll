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

#include "value.h"

BoolExpr* evaluateBoolExpr(BoolExpr* be);
Value* evaluateFuncDef(FuncDef* fd, List* arglist);
Value* evaluateFuncVal(FuncVal* fv);
List* evaluateList(List* l);
Value* evaluateValue(Value* v);
double evaluateValueAsBool(Value* v);
double valueToDouble(Value* v);
