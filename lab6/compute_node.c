//  Hello World client
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mq_node.h"
#include "error_handler.h"
#define BUFFER_LEN 128

void cmd_create(char* node_id, mq_node* compute_node, char* message); // create node_id parent_id
void cmd_remove(char* node_id, mq_node* compute_node, char* message); // remove node_id
void cmd_exec(char* node_id, mq_node* compute_node, char* message); // exec node_id name value
void cmd_ping(char* node_id, mq_node* compute_node, char* message); // ping
void cmd_reconnect(char* node_id, mq_node* compute_node, char* message); // reconnect old_address new_address
void cmd_disconnect(char* node_id, mq_node* compute_node, char* message); // disconnect address

// prog id address
int main(int argc, char** argv){
    if (argc != 3) {
        printf("compute node initial error: invalid number of arguments");
        exit(1);
    }
    char* node_id = argv[1];
    char* address = argv[2];

    mq_node compute_node;
    error_handler(mqn_init(&compute_node));
    error_handler(mqn_bind(&compute_node, address));

    char* buffer = NULL;
    while (1) {
        mqn_receive(&compute_node, &buffer);
        if (!strncmp(buffer, "exec", 4))
            cmd_exec(node_id, &compute_node, buffer);
        else if (!strncmp(buffer, "ping", 4))
            cmd_ping(node_id, &compute_node, buffer);
        else if (!strncmp(buffer, "create", 6))
            cmd_create(node_id, &compute_node, buffer);
        else if (!strncmp(buffer, "remove", 6))
            cmd_remove(node_id, &compute_node, buffer);
        else if (!strncmp(buffer, "reconnect", 9))
            cmd_reconnect(node_id, &compute_node, buffer);
        else if (!strncmp(buffer, "disconnect", 10))
            cmd_disconnect(node_id, &compute_node, buffer);
        free(buffer);
    }
    return 0;
}

void cmd_create(char* node_id, mq_node* compute_node, char* message) {

}
void cmd_remove(char* node_id, mq_node* compute_node, char* message);
void cmd_exec(char* node_id, mq_node* compute_node, char* message);
void cmd_ping(char* node_id, mq_node* compute_node, char* message);
void cmd_reconnect(char* node_id, mq_node* compute_node, char* message);
void cmd_disconnect(char* node_id, mq_node* compute_node, char* message);