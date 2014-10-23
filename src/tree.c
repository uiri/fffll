/* VarTree - Self-balancing binary search tree implementation
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

VarTree* recurDeleteInTree(VarTree* vt) {
  VarTree* t;
  if (vt) {
    if (vt->left) {
      if (!vt->right || (!vt->left->right && !vt->right->left && !vt->right->right)) {
	vt->key = vt->left->key;
	vt->data = vt->left->data;
	vt->left = recurDeleteInTree(vt->left);
	vt->count = 0;
	if (vt->left)
	  vt->count = vt->left->count;
	if (vt->right && vt->right->count > vt->count)
	  vt->count = vt->right->count;
	if (vt->right || vt->left) vt->count++;
      } else if (vt->right->left && (vt->right->right || !vt->left->right)) {
	t = vt->right;
	vt->key = t->left->key;
	vt->data = t->left->data;
	t->left = recurDeleteInTree(t->left);
	t->count = 0;
	if (t->left)
	  t->count = t->left->count;
	if (t->right && t->right->count > t->count) {
	  if (t->right->count - t->count > 1) {
	    vt->right = t->right;
	    t->right = vt->right->left;
	    vt->right->left = t;
	    t = vt->right;
	    t->count = 0;
	    if (t->left)
	      t->count = t->left->count;
	    if (t->right && t->right->count > t->count)
	      t->count = t->right->count;
	  } else {
	    t->count = t->right->count;
	  }
	}
	if (t->right || t->left) t->count++;
      } else if (vt->left->right) {
	t = vt->left;
	vt->key = t->right->key;
	vt->data = t->right->data;
	t->right = recurDeleteInTree(vt->left->right);
	t->count = 0;
	if (t->right)
	  t->count = t->right->count;
	if (t->left && t->left->count > t->count) {
	  if (t->left->count - t->count > 1) {
	    vt->left = t->left;
	    t->left = vt->left->right;
	    vt->left->right = t;
	    t = vt->left;
	    t->count = 0;
	    if (t->left)
	      t->count = t->left->count;
	    if (t->right && t->right->count > t->count)
	      t->count = t->right->count;
	  } else {
	    t->count = t->left->count;
	  }
	}
	if (t->left || t->right)
	  t->count++;
      } else {
	vt->key = vt->right->key;
	vt->data = vt->right->data;
	vt->right = recurDeleteInTree(vt->right);
	vt->count = 0;
	if (vt->right) vt->count = vt->right->count;
	if (vt->left->count > vt->count)
	  vt->count = vt->left->count;
	vt->count++;
      }
    } else if (vt->right) {
      vt->key = vt->right->key;
      vt->data = vt->right->data;
      vt->right = recurDeleteInTree(vt->right);
      vt->count = 0;
      if (vt->right) vt->count = vt->right->count + 1;
    } else {
      free(vt);
      return NULL;
    }
  }
  return vt;
}

VarTree* deleteDataInTree(VarTree* vt, void* data) {
  if (vt == NULL)
    return NULL;
  vt->left = deleteDataInTree(vt->left, data);
  vt->right = deleteDataInTree(vt->right, data);
  if (vt->data == data)
    vt = recurDeleteInTree(vt);
  return vt;
}

VarTree* deleteInTree(VarTree* vt, char* key) {
  if (vt) {
    if (vt->key == key) {
      vt = recurDeleteInTree(vt);
   } else if (vt->key < key) {
      /* rebalance */
      vt->left = deleteInTree(vt->left, key);
    } else {
      /* rebalance */
      vt->right = deleteInTree(vt->right, key);
    }
  }
  return vt;
}

static inline VarTree* findNodeInTree(VarTree* vt, char* key) {
  while (vt) {
    if (vt->key < key) {
      vt = vt->right;
    } else if (vt->key > key) {
      vt = vt->left;
    } else {
      break;
    }
  }
  return vt;
}

void* findInTree(VarTree* vt, char* key) {
  vt = findNodeInTree(vt, key);
  if (vt)
    return vt->data;
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

VarTree* recurInsertInTree(VarTree* vt, char* key, void* data) {
  VarTree* t;
  if (key < vt->key) {
    if (vt->left == NULL) {
      vt->left = newTree(key, data);
      if (!vt->count) vt->count++;
    } else {
      vt->left = recurInsertInTree(vt->left, key, data);
      if (vt->left->count == vt->count) {
        if (!vt->right || vt->left->count - vt->right->count > 1) {
	  t = vt->left;
	  if (t->right && (!t->left || t->right->count > t->left->count)) {
	    t = t->right;
	    vt->left->right = t->left;
	    t->left = vt->left;
	    if (vt->left->right) {
	      vt->left->count = vt->left->right->count;
	      if (vt->left->left && vt->left->left->count > vt->left->count)
		vt->left->count = vt->left->left->count;
	    } else if (vt->left->left) {
	      vt->left->count = vt->left->left->count;
	    } else {
	      vt->left->count = -1;
	    }
	    vt->left->count++;
	  }
	  vt->left = t->right;
	  t->right = vt;
	  if (vt->right) {
	    vt->count = vt->right->count + 1;
	    if (vt->left && vt->left->count > vt->right->count) {
	      vt->count = vt->left->count + 1;
	    }
	  } else if (vt->left) {
	    vt->count = vt->left->count + 1;
	  } else {
	    vt->count = 0;
	  }
	  vt = t;
	  vt->count = vt->right->count;
	}
	vt->count++;
      }
    }
  } else {
    if (vt->right == NULL) {
      vt->right = newTree(key, data);
      if (!vt->count) vt->count++;
    } else {
      vt->right = recurInsertInTree(vt->right, key, data);
      if (vt->right->count == vt->count) {
	if (!vt->left || vt->right->count - vt->left->count > 1) {
	  t = vt->right;
	  if (t->left && (!t->right || t->left->count > t->right->count)) {
	    t = t->left;
	    vt->right->left = t->right;
	    t->right = vt->right;
	    if (vt->right->right) {
	      vt->right->count = vt->right->right->count;
	      if (vt->right->left && vt->right->left->count > vt->right->count)
		vt->right->count = vt->right->left->count;
	    } else if (vt->right->left) {
	      vt->right->count = vt->right->left->count;
	    } else {
	      vt->right->count = -1;
	    }
	    vt->right->count++;
	  }
	  vt->right = t->left;
	  t->left = vt;
	  if (vt->left) {
	    vt->count = vt->left->count + 1;
	    if (vt->right && vt->right->count > vt->left->count) {
	      vt->count = vt->right->count + 1;
	    }
	  } else if (vt->right) {
	    vt->count = vt->right->count + 1;
	  } else {
	    vt->count = 0;
	  }
	  vt = t;
	  vt->count = vt->left->count;
	}
	vt->count++;
      }
    }
  }
  return vt;
}

VarTree* insertInTree(VarTree* vt, char* key, void* data) {
  VarTree* t;
  t = findNodeInTree(vt, key);
  if (t) {
    t->data = data;
    return vt;
  }
  return recurInsertInTree(vt, key, data);
}

VarTree* newTree(char* key, void* data) {
  VarTree* vt;
  vt = malloc(sizeof(VarTree));
  vt->left = NULL;
  vt->right = NULL;
  vt->key = key;
  vt->data = data;
  vt->count = 0;
  return vt;
}
