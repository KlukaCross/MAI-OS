/*
 * Copyright (c) 2019 xieqing. https://github.com/xieqing
 * May be freely redistributed, but copyright notice must be retained.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "avl_data.h"

compute_node_data *makedata(int id, int pid)
{
    compute_node_data *p;

	p = (compute_node_data *) malloc(sizeof(compute_node_data));
	if (p != NULL) {
        p->id = id;
        p->pid = pid;
    }

	return p;
}

int compare_func(const void *d1, const void *d2)
{
    compute_node_data *p1, *p2;
	
	assert(d1 != NULL);
	assert(d2 != NULL);
	
	p1 = (compute_node_data *) d1;
	p2 = (compute_node_data *) d2;
	if (p1->id == p2->id)
		return 0;
	else if (p1->id > p2->id)
		return 1;
	else
		return -1;
}

void destroy_func(void *d)
{
    compute_node_data *p;
	
	assert(d != NULL);
	
	p = (compute_node_data *) d;
	free(p);
}

void print_func(void *d)
{
    compute_node_data *p;
	
	assert(d != NULL);
	
	p = (compute_node_data *) d;
	printf("%d:%d", p->id, p->pid);
}
