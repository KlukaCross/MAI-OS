#include "rm_list.h"

static listel* listel_xor(listel *l1, listel *l2) {
    return (listel *) ((long int) l1 ^ (long int) l2);
}

static listel* get_next(list_iter *it) {
    return listel_xor(it->cur->pn, it->prev);
}

static listel* get_prev_before(list_iter *it) {
    return listel_xor(it->cur, it->prev->pn);
}

void list_init(rm_list *l) {
    l->barrier.pn = NULL;
    l->first = &l->barrier;
    l->size = 0;
}
void list_destroy(rm_list *l) {
    list_iter it = list_iter_begin(l);
    while (!list_iter_equal(it, list_iter_end(l))) {
        list_iter_remove(&it);
    }
}
int get_size(rm_list *l) {
    return l->size;
}

list_iter list_iter_begin(rm_list *l) {
    list_iter it;
    it.lst = l;
    it.prev = &l->barrier;
    it.cur = l->first;
    return it;
}

list_iter list_iter_end(rm_list *l) {
    list_iter it;
    it.lst = l;
    it.cur = &l->barrier;
    it.prev = listel_xor(l->barrier.pn, l->first);
    return it;
}

bool list_iter_equal(list_iter it1, list_iter it2) {
    return it1.prev == it2.prev && it1.cur == it2.cur && it1.lst == it2.lst;
}

void list_iter_move_next(list_iter *it) {
    listel *next = get_next(it);
    it->prev = it->cur;
    it->cur = next;
}

remove_struct list_iter_get(list_iter *it) {
    return it->cur->val;
}

void list_iter_set(list_iter *it, remove_struct val) {
    it->cur->val=val;
}

void list_iter_insert_before(list_iter *it, remove_struct val) {
    listel* new_el = malloc(sizeof(listel));
    new_el->pn = listel_xor(it->cur, it->prev);
    new_el->val = val;
    listel* next = get_next(it);
    if (get_size(it->lst) > 0) {
        it->prev->pn = listel_xor(get_prev_before(it), new_el);
        it->cur->pn = listel_xor(new_el, next);
    }
    it->prev = new_el;
    it->lst->size++;
    if (it->cur == it->lst->first) {
        it->lst->first = new_el;
        // if new_el is only element
        // if (get_size(it->lst) == 1) {
        //     it->prev = it->cur;
        //     it->cur = new_el;
        // }
    }
}

void list_iter_remove(list_iter *it) {
    listel* next = get_next(it);
    if (get_size(it->lst) > 0) {
        it->prev->pn = listel_xor(get_prev_before(it), next);
        next->pn = listel_xor(listel_xor(next->pn, it->cur), it->prev);
    }
    if (it->lst->first == it->cur) {
        it->lst->first = next;
    }
    free(it->cur);
    it->cur = next;
    it->lst->size--;
}