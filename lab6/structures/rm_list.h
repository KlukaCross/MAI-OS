#ifndef LIST_H
#define LIST_H
#include <stdlib.h>
#include <stdbool.h>
#include "balanced_tree.h"

typedef struct {
    int _len_commands;
    int remove_node_id;
    char **commands;
    int number_commands;
    int now_command;
} remove_struct;

typedef struct listel listel;
struct listel {
    remove_struct val;
    listel *pn;
};

typedef struct {
    listel barrier;
    listel* first;
    int size;
} rm_list;

void list_init(rm_list *l);
void list_destroy(rm_list *l);
int get_size(rm_list *l);

typedef struct {
    rm_list *lst;
    listel *prev;
    listel *cur;
} list_iter;

list_iter list_iter_begin(rm_list *l);
list_iter list_iter_end(rm_list *l);
bool list_iter_equal(list_iter it1, list_iter it2);
void list_iter_move_next(list_iter *it);
remove_struct list_iter_get(list_iter *it);

void list_iter_set(list_iter *it, remove_struct val);
void list_iter_insert_before(list_iter *it, remove_struct val);
void list_iter_remove(list_iter *it);

#endif
