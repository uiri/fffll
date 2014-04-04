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

#include <stdlib.h>
#include <string.h>
#include "tree.h"

VarTree* copyTree(VarTree* vt) {
  VarTree* nt;
  if (vt == NULL)
    return NULL;
  nt = newTree(vt->key, vt->data);
  nt->left = copyTree(vt->left);
  nt->right = copyTree(vt->right);
  return nt;
}

VarTree* deleteDataInTree(VarTree* vt, void* data) {
  VarTree* ret;
  if (vt == NULL)
    return NULL;
  if (vt->data == data) {
    ret = mergeTree(deleteDataInTree(vt->right, data), deleteDataInTree(vt->left, data));
    free(vt);
    return ret;
  }
  vt->right = deleteDataInTree(vt->right, data);
  vt->left = deleteDataInTree(vt->left, data);
  return rebalanceTree(vt);
}

VarTree* deleteInTree(VarTree* vt, char* key) {
  VarTree* ret, *t, *u;
  int i;
  ret = NULL;
  t = vt;
  if (t == NULL)
    return vt;
  while (1) {
    u = NULL;
    if (t == NULL)
      break;
    t->count--;
    if (t->left != NULL && key == t->left->key) {
      i = 1;
      u = t->left;
    } else if (t->right != NULL && key == t->right->key) {
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
    if (t == NULL)
      break;
    if (key < t->key) {
      t = t->left;
    } else if (key > t->key) {
      t = t->right;
    }
  }
  return rebalanceTree(vt);
}

void* findInTree(VarTree* vt, char* key) {
  while (vt != NULL) {
    if (key == vt->key)
      return vt->data;
    else if (vt->key > key)
      vt = vt->left;
    else
      vt = vt->right;
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
  VarTree* t;
  t = vt;
  while (1) {
    if (key == t->key) {
      t->data = data;
      break;
    }
    t->count++;
    if (key < t->key) {
      if (t->left == NULL) {
	t->left = newTree(key, data);
	break;
      } else {
	t = t->left;
      }
    } else {
      if (t->right == NULL) {
	t->right = newTree(key, data);
	break;
      } else {
	t = t->right;
      }
    }
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
  if (left == NULL || right == NULL) {
    if (3 > root->count)
      return root;
    root->count = 1;
    if (left != NULL) {
      root->left = NULL;
      return mergeTree(left, root);
    }
    if (right != NULL) {
      root->right = NULL;
      return mergeTree(root, right);
    }
    /* Just in case */
    return root;
  }
  if ((2 > left->count - right->count && left->count - right->count > -1) ||
      (2 > right->count - left->count && right->count - left->count > -1))
    return root;
  if (left->count > right->count) {
    root->count = 1;
    root->left = NULL;
    root->right = NULL;
    right->left = mergeTree(root, right->left);
    left->right = mergeTree(left->right, right);
    root = left;
  } else {
    root->count = 1;
    root->left = NULL;
    root->right = NULL;
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
