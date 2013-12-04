/* VarTree - Self-balancing binary search tree implementation
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

#ifndef VARTREE
#define VARTREE

typedef struct VarTree VarTree;

struct VarTree {
  VarTree* left;
  VarTree* right;
  char* key;
  void* data;
  int count;
};

#endif

VarTree* copyTree(VarTree* vt);
VarTree* deleteInTree(VarTree* vt, char* key);
void* findInTree(VarTree* vt, char* key);
int freeTree(VarTree* vt);
VarTree* insertInTree(VarTree* vt, char* key, void* data);
VarTree* mergeTree(VarTree* left, VarTree* right);
VarTree* newTree(char* key, void* data);
VarTree* rebalanceTree(VarTree* vt);
