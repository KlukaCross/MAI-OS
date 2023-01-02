#include "balanced_tree.h"

#define K_SIZE 2

void btree_init(b_tree *t) {
    t->size = 0;
    t->pool_size = 0;
    t->buf = NULL;
}

void btree_destroy(b_tree *t) {
    t->size = 0;
    t->pool_size = 0;
    free(t->buf);
    t->buf = NULL;
}

static bool set_size(b_tree *t, size_t new_size) {
    if (new_size < t->size) {
        t->size = new_size;
        if (t->size * 2*K_SIZE <= t->pool_size) {
            t->pool_size = t->pool_size / K_SIZE;
            t->buf = realloc(t->buf, t->pool_size * sizeof(b_tree_node));
        }
    }
    else if (new_size > t->size) {
        t->size = new_size;
        if (new_size > t->pool_size) {
            size_t new_pool_size = t->pool_size * K_SIZE;
            if (t->pool_size == 0) new_pool_size = K_SIZE * 2;
            b_tree_node *new_buf = realloc(t->buf, new_pool_size * sizeof(b_tree_node));
            if (new_buf == NULL) return false;
            t->pool_size = new_pool_size;
            t->buf = new_buf;
        }
    }
    return true;
}

bool btree_add(b_tree *t, b_tree_node elem) {
    if (!set_size(t, t->size+1)) return false;
    t->buf[t->size-1] = elem;
    return true;
}

b_tree_node btree_last_elem_parent(b_tree *t) {
    return t->buf[(t->size-2)/2];
}

b_tree_node btree_last_elem(b_tree *t) {
    return t->buf[t->size-1];
}

b_tree_node btree_remove(b_tree *t, int elem_id) {
    b_tree_node r_elem = t->buf[t->size-1];

    for (int i = 0; i < t->size; ++i) {
        if (t->buf[i].id == elem_id) {
            t->buf[i] = r_elem;
            break;
        }
    }

    t->size--;
    return r_elem;
}

b_tree_node btree_find(b_tree *t, int elem_id) {
    b_tree_node res = {-1, -1, false};
    for (int i = 0; i < t->size; ++i) {
        if (t->buf[i].id == elem_id) {
            res = t->buf[i];
            break;
        }
    }
    return res;
}

b_tree_node btree_get_parent(b_tree *t, int elem_id) {
    b_tree_node res = {-1, -1, false};
    for (int i = 0; i < t->size; ++i) {
        if (t->buf[i].id == elem_id) {
            if (i == 0)
                return res;
            res = t->buf[(i-1)/2];
            break;
        }
    }
    return res;
}

b_tree_node btree_get_left(b_tree *t, int elem_id) {
    b_tree_node res = {-1, -1, false};
    for (int i = 0; i < t->size; ++i) {
        if (t->buf[i].id == elem_id) {
            if (2*i+1 >= t->size)
                return res;
            res = t->buf[2*i+1];
            break;
        }
    }
    return res;
}
b_tree_node btree_get_right(b_tree *t, int elem_id) {
    b_tree_node res = {-1, -1, false};
    for (int i = 0; i < t->size; ++i) {
        if (t->buf[i].id == elem_id) {
            if (2*i+2 >= t->size)
                return res;
            res = t->buf[2*i+2];
            break;
        }
    }
    return res;
}