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
  VarTree* ret;
  int i;
  if (key == vt->key) {
    if (vt->left == NULL && vt->right == NULL) {
      freeTree(vt);
      return NULL;
    }
    if (vt->left == NULL) {
      ret = vt->right;
      free(vt);
      return ret;
    }
    if (vt->right == NULL) {
      ret = vt->left;
      free(vt);
      return ret;
    }
    ret = mergeTree(vt->left, vt->right);
    free(vt);
    return ret;
  }
  for (i=0;key[i] != '\0' && key[i] == vt->key[i];i++);
  if (key[i] < vt->key[i]) {
    vt->left = deleteInTree(vt->left, key);
  }
  if (key[i] > vt->key[i]) {
    vt->right = deleteInTree(vt->right, key);
  }
  return rebalanceTree(vt);
}

void* findInTree(VarTree* vt, char* key) {
  int i;
  if (vt == NULL)
    return NULL;
  if (key == vt->key) {
    return vt->data;
  }
  for (i=0;key[i] != '\0' && key[i] == vt->key[i];i++);
  if (key[i] < vt->key[i]) {
    return findInTree(vt->left, key);
  }
  if (key[i] > vt->key[i]) {
    return findInTree(vt->right, key);
  }
  return NULL;
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
  if (left == NULL || (right != NULL && left->count < right->count)) {
    root = right;
    root->left = mergeTree(left, right->left);
  } else {
    root = left;
    root->right = mergeTree(left->right, right);
  }
  return rebalanceTree(root);
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
  if ((right == NULL && 
       left->count > 1) ||
      (right != NULL && left != NULL && left->count - right->count > 1)) {
    root->left = left->right;
    left->right = root;
    root = left;
    left = root->left;
    if (left == NULL && right == NULL)
      root->count = 1;
    else if (left == NULL)
      root->count = root->right->count + 1;
    else {
      if (left->right == NULL && left->left == NULL) {
	left->count = 1;
      } else if (left->left == NULL) {
	left->count = left->right->count + 1;
      } else if (left->right == NULL) {
	left->count = left->left->count + 1;
      } else {
	left->count = left->left->count + left->right->count + 1;
      }
      if (right == NULL)
	root->count = left->count + 1;
      else
	root->count = left->count + right->count + 1;
    }
  } else if ((left == NULL && right->count > 1) || (right != NULL && left != NULL && right->count - left->count > 1)) {
    root->right = right->left;
    right->left = root;
    root = right;
    right = root->right;
    left = root->left;
    if (right == NULL && left == NULL)
      root->count = 1;
    else if (right == NULL)
      root->count = left->count + 1;
    else {
      if (right->left == NULL && right->right == NULL) {
	right->count = 1;
      } else if (right->left == NULL) {
	right->count = right->right->count + 1;
      } else if (right->right == NULL) {
	right->count = right->left->count + 1;
      } else {
	right->count = right->left->count + right->right->count + 1;
      }
      if (left == NULL)
	root->count = right->count + 1;
      else
	root->count = left->count + right->count + 1;
    }
  }
  return root;
}
