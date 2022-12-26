//  Hello World client
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mq_node.h"
#include "error_handler.h"
#define BUFFER_LEN 128

void cmd_create(char* message); // create NODE_ID parent_id node_main_address node_ping_address
void cmd_remove(char* message); // remove NODE_ID
void cmd_exec(char* message); // exec NODE_ID name value
void cmd_ping(char* message); // ping NODE_ID
void cmd_connect(char* message); // connect NODE_ID main_address ping_address
void cmd_disconnect(char* message); // disconnect main_address

char* NODE_ID;
mq_node COMPUTE_NODE;

// prog id main_address ping_address
int main(int argc, char** argv){
    if (argc != 4) {
        printf("compute node initial error: invalid number of arguments");
        exit(1);
    }
    NODE_ID = argv[1];
    char* main_address = argv[2];
    char* ping_address = argv[2];

    error_handler(mqn_init(&COMPUTE_NODE));
    error_handler(mqn_bind(&COMPUTE_NODE, main_address, ping_address));

    char* buffer = NULL;
    while (1) {
        mqn_receive(&COMPUTE_NODE, &buffer);
        if (!strncmp(buffer, "exec", 4))
            cmd_exec(buffer);
        else if (!strncmp(buffer, "create", 6))
            cmd_create(buffer);
        else if (!strncmp(buffer, "remove", 6))
            cmd_remove(buffer);
        else if (!strncmp(buffer, "connect", 7))
            cmd_connect(buffer);
        else if (!strncmp(buffer, "disconnect", 10))
            cmd_disconnect(buffer);
        free(buffer);
    }
    return 0;
}

void cmd_create(char* message) {
    char *node_id = NULL;
    char *parent_id = NULL;
    char *node_main_address = NULL;
    char *node_ping_address = NULL;
    sscanf(message, "create %ms %ms %ms %ms", &node_id, &parent_id, &node_main_address, &node_ping_address);
    if (strcmp(NODE_ID, parent_id) != 0 && strcmp(NODE_ID, node_id) != 0) { // отправляем дальше по узлам
        char* answer = mqn_left_push(&COMPUTE_NODE, message);
        if (!is_ok(answer))
            answer = mqn_right_push(&COMPUTE_NODE, message);
        // отправим назад answer
    } else if (!strcmp(NODE_ID, parent_id)) { // это родитель добавляемого узла

    } else {// это добавляемый узел

    }

    free(node_id);
    free(parent_id);
    free(node_main_address);
    free(node_ping_address);
}
void cmd_remove(char* message);
void cmd_exec(char* message);
void cmd_ping(char* message);
void cmd_reconnect(char* message);
void cmd_disconnect(char* message);