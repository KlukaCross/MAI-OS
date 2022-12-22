//  Hello World server
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "balanced_tree.h"
#include "mq_node.h"
#include "error_handler.h"
#include "d_string.h"

char* get_address(int *address_shift);

void cmd_create(mq_node* control_node, char* message); // create id
void cmd_remove(mq_node* control_node, char* message); // remove id
void cmd_exec(mq_node* control_node, char* message); // exec id name value
void cmd_heartbit(mq_node* control_node, char* message); // heartbit time

int main (void)
{
    b_tree tree;
    btree_init(&tree);
    b_tree_node c_node = {0, getpid(), true};
    btree_add(&tree, c_node);

    int address_shift = 0;
    char* main_address = get_address(&address_shift);
    mq_node control_node;

    error_handler(mqn_init(&control_node));
    error_handler(mqn_bind(&control_node, main_address));

    char* buffer = NULL;
    int* buffer_len = NULL;
    while (1) {
        scan_string(&buffer, buffer_len);
        if (!(strncmp(buffer, "exec", 4)))
            cmd_exec(&control_node, buffer);
        if (!(strncmp(buffer, "create", 6)))
            cmd_create(&control_node, buffer);
        if (!(strncmp(buffer, "remove", 6)))
            cmd_remove(&control_node, buffer);
        if (!(strncmp(buffer, "heartbit", 8)))
            cmd_heartbit(&control_node, buffer);
        free(buffer);
    }
    return 0;
}

char* get_address(int *address_shift) {
    char buff[12];
    char* res;
    sprintf(buff, "%d", (*address_shift)++);
    res = strcat("tcp://*:", buff);
    return res;
}