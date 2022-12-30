#ifndef LAB6_TREE_H
#define LAB6_TREE_H
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    int id;
    int pid;
    char* main_address;
    char* ping_address;
} b_tree_node;

typedef struct {
    b_tree_node *buf;
    size_t pool_size;
    size_t size;
} b_tree;

void btree_init(b_tree *t);
void btree_destroy(b_tree *t);

bool btree_add(b_tree *t, b_tree_node elem);
b_tree_node btree_last_elem_parent(b_tree *t);
b_tree_node btree_last_elem(b_tree *t);

b_tree_node btree_remove(b_tree *t, int elem_id); // return node that will stand in place of the one being deleted

b_tree_node btree_find(b_tree *t, int elem_id);
b_tree_node btree_get_parent(b_tree *t, int elem_id);
b_tree_node btree_get_left(b_tree *t, int elem_id);
b_tree_node btree_get_right(b_tree *t, int elem_id);

#endif //LAB6_TREE_H
