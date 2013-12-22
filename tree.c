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

#include "tree.h"
#include <stdlib.h>
#include <string.h>

VarTree* copyTree(VarTree* vt) {
  VarTree* nt;
  if (vt == NULL)
    return NULL;
  nt = newTree(vt->key, vt->data);
  nt->left = copyTree(vt->left);
  nt->right = copyTree(vt->right);
  return nt;
}

VarTree* deleteInTree(VarTree* vt, char* key) {
  VarTree* ret, *t, *u;
  int i;
  ret = NULL;
  t = vt;
  while (1) {
    u = NULL;
    t->count--;
    if (key == t->left->key) {
      i = 1;
      u = t->left;
    } else if (key == t->right->key) {
      i = 0;
      u = t->right;
    }
    if (u != NULL) {
      if (u->left != NULL || u->right != NULL) {
	ret = mergeTree(u->left, u->right);
      }
      free(u);
      if (i)
	t->left = ret;
      else
	t->right = ret;
      break;
    }
    for (i=0;key[i] != '\0' && key[i] == t->key[i];i++);
    if (key[i] < t->key[i]) {
      t = t->left;
    }
    if (key[i] > t->key[i]) {
      t = t->right;
    }
  }
  return rebalanceTree(vt);
}

void* findInTree(VarTree* vt, char* key) {
  int i;
  VarTree* t;
  t = vt;
  while (1) {
    if (t == NULL)
      return NULL;
    if (key == t->key)
      return t->data;
    for (i=0;key[i] != '\0' && key[i] == t->key[i];i++);
    if (key[i] == '\0')
      return NULL;
    else if (key[i] < t->key[i]) {
      t = t->left;
    } else {
      t = t->right;
    }
  }
}

int freeTree(VarTree* vt) {
  if (vt == NULL)
    return 0;
  freeTree(vt->left);
  freeTree(vt->right);
  free(vt);
  return 1;
}

VarTree* insertInTree(VarTree* vt, char* key, void* data) {
  int i;
  if (key == vt->key) {
    vt->data = data;
    return vt;
  }
  for (i=0;key[i] != '\0' && key[i] == vt->key[i];i++);
  if (key[i] < vt->key[i]) {
    vt->count++;
    if (vt->left == NULL)
      vt->left = newTree(key, data);
    else
      vt->left = insertInTree(vt->left, key, data);
  } else if (key[i] > vt->key[i]) {
    vt->count++;
    if (vt->right == NULL)
      vt->right = newTree(key, data);
    else
      vt->right = insertInTree(vt->right, key, data);
  }
  return rebalanceTree(vt);
}

VarTree* mergeTree(VarTree* left, VarTree* right) {
  VarTree* root;
  if (left == NULL && right == NULL) {
    return NULL;
  }
  if (left == NULL)
    return right;
  if (right == NULL)
    return left;
  if (left->count < right->count) {
    root = right;
    root->left = mergeTree(left, right->left);
  } else {
    root = left;
    root->right = mergeTree(left->right, right);
  }
  root->count = 1;
  if (root->left != NULL)
    root->count += root->left->count;
  if (root->right != NULL)
    root->count += root->right->count;
  return root;
}

VarTree* newTree(char* key, void* data) {
  VarTree* vt;
  vt = malloc(sizeof(VarTree));
  vt->left = NULL;
  vt->right = NULL;
  vt->key = key;
  vt->data = data;
  vt->count = 1;
  return vt;
}

VarTree* rebalanceTree(VarTree* vt) {
  VarTree* root, *left, *right;
  if (vt == NULL)
    return NULL;
  root = vt;
  left = vt->left;
  right = vt->right;
  if (right == NULL && left == NULL) {
    root->count = 1;
    return root;
  }
  if (right == NULL) {
    root->count = 1;
    root->left = NULL;
    return mergeTree(left, root);
  }
  if (left == NULL) {
    root->count = 1;
    root->right = NULL;
    return mergeTree(root, right);
  }
  root->count = 1;
  root->left = NULL;
  root->right = NULL;
  if (left->count - right->count > 1) {
    right->left = mergeTree(root, right->left);
    left->right = mergeTree(left->right, right);
    root = left;
  } else {
    left->right = mergeTree(left->right, root);
    right->left = mergeTree(left, right->left);
    root = right;
  }
  root->count = 1;
  if (root->right != NULL)
    root->count += root->right->count;
  if (root->left != NULL)
    root->count += root->left->count;
  return root;
}
