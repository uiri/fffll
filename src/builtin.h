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

#include "value.h"

Value* addDef(FuncDef* fd, List* arglist);
Value* catDef(FuncDef* fd, List* arglist);
Value* dieDef(FuncDef* fd, List* arglist);
Value* forDef(FuncDef* fd, List* arglist);
Value* headDef(FuncDef* fd, List* arglist);
Value* ifDef(FuncDef* fd, List* arglist);
Value* lenDef(FuncDef* fd, List* arglist);
Value* mulDef(FuncDef* fd, List* arglist);
Value* openDef(FuncDef* fd, List* arglist);
Value* pushDef(FuncDef* fd, List* arglist);
Value* rcpDef(FuncDef* fd, List* arglist);
Value* readDef(FuncDef* fd, List* arglist);
Value* saveDef(FuncDef* fd, List* arglist);
Value* setDef(FuncDef* fd, List* arglist);
Value* tailDef(FuncDef* fd, List* arglist);
Value* tokDef(FuncDef* fd, List* arglist);
Value* writeDef(FuncDef* fd, List* arglist);
