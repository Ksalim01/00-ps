#include <stdlib.h>
#include <solution.h>

typedef int KeyType;
typedef unsigned int SizeType;

struct BTreeNode {
  struct BTreeNode* parent;
  KeyType* keys;
  struct BTreeNode** children;
  SizeType size;
};

struct Splitted {
  struct BTreeNode* left;
  struct BTreeNode* right;
  KeyType middle;
};

struct btree {
  struct BTreeNode* root;
  SizeType lower_factor;
  SizeType upper_factor;
};

struct Splitted SplitInHalf(struct BTreeNode* node, SizeType upper_factor);
struct BTreeNode* BTreeNodeFrom(struct BTreeNode* node, SizeType left, SizeType right, SizeType upper_factor);
void ShiftRight(struct BTreeNode* node, SizeType index);
void InsertInNode(struct BTreeNode* node, struct Splitted splitted);

void ClearOnlyNode(struct BTreeNode* node) {
  if (node == NULL) {
    return;
  }

  node->size = 0;
  //   delete[] node->keys;
  //   delete[] node->children;
  //   delete node;
  free(node->keys);
  free(node->children);
  free(node);
}

struct BTreeNode* NewBTreeNode(SizeType size, SizeType max_size) {
  struct BTreeNode* new_node = (struct BTreeNode*)malloc(sizeof(struct BTreeNode));  // new BTreeNode();
  new_node->parent = NULL;
  // std::cout << "new node\n";
  new_node->size = size;
  new_node->keys = (KeyType*)malloc(sizeof(KeyType) * max_size);  // new KeyType[max_size];
  new_node->children = (struct BTreeNode**)malloc(sizeof(struct BTreeNode*) *
                                                  (max_size + 1));  // new struct BTreeNode* [max_size + 1] { NULL };
  for (SizeType i = 0; i < max_size + 1; ++i) {
    new_node->children[i] = NULL;
  }
  return new_node;
  //   struct BTreeNode* new_node = new BTreeNode();
  //   new_node->parent = NULL;
  //   // std::cout << "new node\n";
  //   new_node->size = size;
  //   new_node->keys = new KeyType[max_size];
  //   new_node->children = new struct BTreeNode* [max_size + 1] { NULL };
  //   for (SizeType i = 0; i < max_size + 1; ++i) {
  //     new_node->children[i] = NULL;
  //   }
  //   return new_node;
}

void SetParent(struct BTreeNode* node, struct BTreeNode* parent) {
  if (node == NULL) {
    return;
  }
  node->parent = parent;
}

SizeType LowerBound(struct BTreeNode* node, KeyType pivot) {
  SizeType i;
  for (i = 0; i < node->size && node->keys[i] < pivot; ++i) {
  }
  return i;
}

SizeType FindChild(struct BTreeNode* parent, struct BTreeNode* child) {
  SizeType i;
  for (i = 0; i <= parent->size && parent->children[i] != child; ++i) {
  }
  return i;
}

void SplitIthChild(struct BTreeNode* node, SizeType index, SizeType upper_factor) {
  if (node->children[index]->size != upper_factor) {
    return;
  }
  struct Splitted splitted = SplitInHalf(node->children[index], upper_factor);
  InsertInNode(node, splitted);
}

void InsertInNode(struct BTreeNode* node, struct Splitted splitted) {
  SizeType lb = LowerBound(node, splitted.middle);
  ShiftRight(node, lb);
  node->keys[lb] = splitted.middle;
  node->children[lb] = splitted.left;
  node->children[lb]->parent = node;
  node->children[lb + 1] = splitted.right;
  node->children[lb + 1]->parent = node;
}

void ShiftRight(struct BTreeNode* node, SizeType index) {
  SizeType prev_key = node->keys[index];
  struct BTreeNode* prev_child = node->children[index];
  for (SizeType i = index + 1; i <= node->size; ++i) {
    KeyType temp_key = node->keys[i];
    struct BTreeNode* temp_child = node->children[i];
    node->keys[i] = prev_key;
    node->children[i] = prev_child;
    prev_key = temp_key;
    prev_child = temp_child;
  }
  node->children[node->size + 1] = prev_child;
  node->children[index] = NULL;
  ++node->size;
}

void ShiftLeft(struct BTreeNode* node, SizeType index) {
  for (SizeType i = index; i < node->size - 1; ++i) {
    node->keys[i] = node->keys[i + 1];
    node->children[i] = node->children[i + 1];
  }
  node->children[node->size - 1] = node->children[node->size];
  node->children[node->size] = NULL;
  --node->size;
}

struct Splitted SplitInHalf(struct BTreeNode* node, SizeType upper_factor) {
  struct Splitted splitted;
  splitted.middle = node->keys[node->size / 2];
  splitted.right = BTreeNodeFrom(node, node->size / 2 + 1, node->size, upper_factor);
  splitted.left = BTreeNodeFrom(node, 0, node->size / 2, upper_factor);
  ClearOnlyNode(node);
  return splitted;
}

struct BTreeNode* BTreeNodeFrom(struct BTreeNode* node, SizeType left, SizeType right, SizeType upper_factor) {
  struct BTreeNode* new_node = NewBTreeNode(right - left, upper_factor);
  for (SizeType i = left; i < right; ++i) {
    new_node->keys[i - left] = node->keys[i];
    new_node->children[i - left] = node->children[i];
    SetParent(new_node->children[i - left], new_node);
  }
  new_node->children[right - left] = node->children[right];
  SetParent(new_node->children[right - left], new_node);
  return new_node;
}

void MergeSplitted(struct BTreeNode* node, struct Splitted splitted) {
  node->keys[0] = splitted.middle;
  node->children[0] = splitted.left;
  node->children[1] = splitted.right;
  node->children[0]->parent = node;
  node->children[1]->parent = node;
}

void Clear(struct BTreeNode* node) {
  if (node == NULL) {
    return;
  }

  for (SizeType i = 0; i <= node->size; ++i) {
    Clear(node->children[i]);
  }

  node->size = 0;
//   delete[] node->keys;
//   delete[] node->children;
//   delete node;
    free(node->keys);      // delete[] node->keys;
    free(node->children);  // delete[] node->children;
    free(node);            // delete node;
}

KeyType FindMax(struct BTreeNode* root) {
  if (root == NULL) {
    return -1;
  }
  if (root->children[root->size] == NULL) {
    return root->keys[root->size - 1];
  }
  return FindMax(root->children[root->size]);
}

KeyType FindMin(struct BTreeNode* root) {
  if (root == NULL) {
    return -1;
  }
  if (root->children[0] == NULL) {
    return root->keys[0];
  }
  return FindMin(root->children[0]);
}

struct BTreeNode* MergeTwoNodes(struct BTreeNode* left, struct BTreeNode* right, KeyType middle) {
  if (left == NULL) {
    ShiftRight(right, 0);
    right->keys[0] = middle;
    right->children[0] = NULL;
    return right;
  }

  if (right == NULL) {
    left->keys[left->size] = middle;
    left->children[left->size + 1] = NULL;
    return left;
  }

  left->keys[left->size] = middle;
  for (SizeType i = 0; i < right->size; ++i) {
    left->keys[left->size + i + 1] = right->keys[i];
    left->children[left->size + i + 1] = right->children[i];
    SetParent(left->children[left->size + i + 1], left);
  }

  left->children[left->size + right->size + 1] = right->children[right->size];
  SetParent(left->children[left->size + right->size + 1], left);
  left->size += right->size + 1;
  ClearOnlyNode(right);
  return left;
}

struct BTreeNode* MergeTwoNeighbours(struct BTreeNode* root, SizeType index, struct btree* t) {
  struct BTreeNode* merged = MergeTwoNodes(root->children[index - 1], root->children[index], root->keys[index - 1]);
  ShiftLeft(root, index - 1);
  if (root->size == 0) {
    t->root = merged;
  } else {
    root->children[index - 1] = merged;
    SetParent(root->children[index - 1], root);
  }
  return merged;
}

void Merge(struct BTreeNode* root, SizeType index, struct btree* t) {
  struct BTreeNode* middle_node = root->children[index];
  if (index > 0 && root->children[index - 1] != NULL && root->children[index - 1]->size > t->lower_factor) {
    struct BTreeNode* left_child = root->children[index - 1];
    ShiftRight(middle_node, 0);
    middle_node->keys[0] = root->keys[index - 1];
    root->keys[index - 1] = left_child->keys[left_child->size - 1];
    middle_node->children[0] = left_child->children[left_child->size];
    SetParent(middle_node->children[0], middle_node);
    --left_child->size;
  } else if (index < root->size && root->children[index + 1] != NULL &&
             root->children[index + 1]->size > t->lower_factor) {
    struct BTreeNode* right_node = root->children[index + 1];
    middle_node->keys[middle_node->size] = root->keys[index];
    root->keys[index] = right_node->keys[0];
    middle_node->children[middle_node->size + 1] = right_node->children[0];
    SetParent(middle_node->children[middle_node->size + 1], middle_node);
    ++middle_node->size;
    ShiftLeft(right_node, 0);
  } else if (index > 0) {
    MergeTwoNeighbours(root, index, t);
  } else {
    MergeTwoNeighbours(root, index + 1, t);
  }
}

void Insert(struct BTreeNode* root, KeyType new_key, struct btree* t) {
  if (root == NULL) {
    root = NewBTreeNode(1, t->upper_factor);
    root->keys[0] = new_key;
    t->root = root;
    return;
  }

  SizeType lb = LowerBound(root, new_key);
  if (lb < root->size && root->keys[lb] == new_key) {
    return;
  }

  if (root->size == t->upper_factor && root == t->root) {
    struct BTreeNode* new_root = NewBTreeNode(1, t->upper_factor);
    MergeSplitted(new_root, SplitInHalf(root, t->upper_factor));
    t->root = new_root;
    Insert(t->root, new_key, t);
  } else if (root->size == t->upper_factor) {
    Insert(root->parent, new_key, t);
  } else if (root->children[lb] != NULL) {
    if (root->children[lb]->size == t->upper_factor) {
      SplitIthChild(root, lb, t->upper_factor);
      Insert(root, new_key, t);
    } else {
      Insert(root->children[lb], new_key, t);
    }
  } else {
    ShiftRight(root, lb);
    root->keys[lb] = new_key;
  }
}

SizeType CountSize(struct BTreeNode* root) {
  if (root == NULL) {
    return 0;
  }

  SizeType sum = root->size;
  for (SizeType i = 0; i <= root->size; ++i) {
    sum += CountSize(root->children[i]);
  }

  return sum;
}

void Traverse(struct BTreeNode* node, KeyType** arr) {
  if (node == NULL)
    return;
  for (SizeType i = 0; i < node->size; ++i) {
    Traverse(node->children[i], arr);
    **arr = node->keys[i];
    ++(*arr);
  }
  Traverse(node->children[node->size], arr);
}

bool Contains(struct BTreeNode* root, KeyType key) {
  if (root == NULL) {
    return false;
  }

  SizeType lb = LowerBound(root, key);
  if (lb < root->size && root->keys[lb] == key) {
    return true;
  }

  return Contains(root->children[lb], key);
}

void Erase(struct BTreeNode* root, KeyType key, struct btree* t) {
  if (root == NULL) {
    return;
  }

  SizeType lb = LowerBound(root, key);
  if (lb < root->size && root->keys[lb] != key && root->children[lb] == NULL) {
    return;
  }
  if (lb == root->size && root->children[lb] == NULL) {
    return;
  }

  if (lb == root->size || root->keys[lb] != key) {
    if (root->children[lb]->size == t->lower_factor) {
      Merge(root, lb, t);
      if (root->size == 0) {
        ClearOnlyNode(root);
        Erase(t->root, key, t);
      } else {
        Erase(root, key, t);
      }
    } else {
      Erase(root->children[lb], key, t);
    }
  } else if (root->children[lb] == NULL) {
    ShiftLeft(root, lb);
  } else if (root->children[lb + 1] == NULL) {
    struct BTreeNode* prev = root->children[lb];
    ShiftLeft(root, lb);
    root->children[lb] = prev;
  } else if (root->children[lb]->size > t->lower_factor) {
    KeyType max_key = FindMax(root->children[lb]);
    Erase(root->children[lb], max_key, t);
    root->keys[lb] = max_key;
  } else if (root->children[lb + 1]->size > t->lower_factor) {
    KeyType min_key = FindMin(root->children[lb + 1]);
    Erase(root->children[lb + 1], min_key, t);
    root->keys[lb] = min_key;
  } else {
    MergeTwoNeighbours(root, lb + 1, t);
    if (root->size == 0) {
      ClearOnlyNode(root);
      Erase(t->root, key, t);
    } else {
      Erase(root, key, t);
    }
  }
}

struct btree* btree_alloc(unsigned int L) {
  struct btree* t = (struct btree*) malloc(sizeof(struct btree));  // new btree();
  t->root = NULL;
  t->lower_factor = L;
  t->upper_factor = 2 * L + 1;
  return t;
}

void btree_free(struct btree* t) {
  Clear(t->root);
  free(t);
}

void btree_insert(struct btree* t, int x) {
  Insert(t->root, x, t);
}

void btree_delete(struct btree* t, int x) {
  Erase(t->root, x, t);
}

bool btree_contains(struct btree* t, int x) {
  return Contains(t->root, x);
}

struct btree_iter {
  KeyType* elements;
  SizeType pos;
  SizeType size;
};

struct btree_iter* btree_iter_start(struct btree* t) {
  struct btree_iter* iter = (struct btree_iter*) malloc(sizeof(struct btree_iter));  // new btree_iter();
  iter->size = CountSize(t->root);
  iter->elements = (KeyType*) malloc(sizeof(KeyType) * iter->size);  // new KeyType[iter->size];
  iter->pos = 0;

  KeyType* copy = iter->elements;
  Traverse(t->root, &copy);
  return iter;
}

void btree_iter_end(struct btree_iter* i) {
  free(i->elements);  // delete[] i->elements;
  free(i);            // delete i;
}

bool btree_iter_next(struct btree_iter* i, int* x) {
  if (i->pos == i->size) {
    return false;
  }

  *x = i->elements[i->pos];
  ++i->pos;
  return true;
}