//  Hello World server
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "balanced_tree.h"
#include "mq_node.h"
#include "error_handler.h"
#include "d_string.h"

char* gen_address();

void cmd_create(char* message); // create id
void cmd_remove(char* message); // remove id
void cmd_exec(char* message); // exec id name value
void cmd_heartbit(char* message); // heartbit time

int address_shift;
b_tree tree;
mq_node control_node;
b_tree_node c_node;

int main (void)
{
    btree_init(&tree);
    c_node.id = "-1";
    c_node.pid = getpid();
    c_node.is_available = true;
    btree_add(&tree, c_node);

    address_shift = 0;
    char* main_address = gen_address(&address_shift);

    error_handler(mqn_init(&control_node));
    error_handler(mqn_bind(&control_node, main_address));

    char* buffer = NULL;
    int* buffer_len = NULL;
    while (1) {
        scan_string(&buffer, buffer_len);
        if (!(strncmp(buffer, "exec", 4)))
            cmd_exec(buffer);
        if (!(strncmp(buffer, "create", 6)))
            cmd_create(buffer);
        if (!(strncmp(buffer, "remove", 6)))
            cmd_remove(buffer);
        if (!(strncmp(buffer, "heartbit", 8)))
            cmd_heartbit(buffer);
        free(buffer);
    }
    return 0;
}

char* gen_address() {
    char buff[12];
    char* res;
    sprintf(buff, "%d", address_shift++);
    res = strcat("tcp://*:", buff);
    return res;
}

void cmd_create(char* message) {
    char* node_id;
    sscanf(message, "create %s", node_id);
    if (btree_find(&tree, node_id).pid != -1) {
        printf("Error: Already exists");
        return;
    }

    char* main_address = gen_address();
    char* ping_address = gen_address();

    int pid = fork();
    if (pid < 0) {
        perror("node create error: ");
        exit(errno);
    } else if (pid == 0) {
        execl("./compute_node", "./compute_node", node_id, main_address, ping_address, (char *) NULL);
        perror("node create error: ");
        exit(errno);
    } else {
        b_tree_node node = {node_id, pid, true};
        btree_add(&tree, node);
        b_tree_node new_node_parent = btree_last_elem_parent(&tree);
        if (!strcmp(new_node_parent.id, c_node.id)) {
            if (!mqn_connect(&control_node, main_address, ping_address))
                printf("Create node error\n");
        }
        else {
            char *push_message;
            sprintf(push_message, "create %s %s", node_id, new_node_parent.id);
            char* answer = mqn_left_push(&control_node, push_message);
            if (parent_not_found(answer))
                answer = mqn_left_push(&control_node, push_message);
            if (!is_ok(push_message))
                btree_remove(&tree, node_id);
            if (parent_not_found(answer))
                printf("Error: Parent %s is unavailable\n", new_node_parent.id);
            else
                printf("%s\n", answer);
        }
    }
}

void cmd_remove(char* message) {
    char* node_id;
    sscanf(message, "create %s", node_id);
    b_tree_node node = btree_find(&tree, node_id);
    if (node.pid == -1) {
        printf("Error: Not found");
        return;
    }


    kill(node.pid, SIGKILL);
}

void cmd_exec(char* message); // exec id name value
void cmd_heartbit(char* message); // heartbit time