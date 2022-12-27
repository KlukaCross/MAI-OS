#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "mq_node.h"
#include "error_handler.h"
#include "hashmap/hashmap.h"

void cmd_connect(char* message); // connect node_id parent_id main_address ping_address
void cmd_remove(char* message); // remove node_id
void cmd_exec(char* message); // exec node_id name value
void cmd_ping(char* message); // ping node_id
void cmd_disconnect(char* message); // disconnect main_address

char* NODE_ID;
mq_node COMPUTE_NODE;

HASHMAP(char*, int) CHI_MAP;

// prog id main_address ping_address
int main(int argc, char** argv){
    if (argc != 4) {
        printf("compute node initial error: invalid number of arguments");
        exit(1);
    }
    NODE_ID = argv[1];
    char* main_address = argv[2];
    char* ping_address = argv[2];
    hashmap_init(&CHI_MAP, hashmap_hash_string, strcmp);

    error_handler(mqn_init(&COMPUTE_NODE));
    error_handler(mqn_bind(&COMPUTE_NODE, main_address, ping_address));

    char* buffer = NULL;
    while (1) {
        mqn_receive(&COMPUTE_NODE, &buffer);
        if (!strncmp(buffer, "exec", 4))
            cmd_exec(buffer);
        else if (!strncmp(buffer, "ping", 4))
            cmd_ping(buffer);
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

void cmd_connect(char* message) {
    char *node_id = NULL;
    char *parent_id = NULL;
    char *node_main_address = NULL;
    char *node_ping_address = NULL;
    sscanf(message, "create %ms %ms %ms %ms", &node_id, &parent_id, &node_main_address, &node_ping_address);
    if (strcmp(NODE_ID, parent_id) != 0 && strcmp(NODE_ID, node_id) != 0) { // отправляем дальше по узлам
        char* answer = mqn_all_push(&COMPUTE_NODE, message);
        (!answer)? mqn_reply(&COMPUTE_NODE, ERROR_NODE_NOT_FOUND) : mqn_reply(&COMPUTE_NODE, answer);
    } else if (!strcmp(NODE_ID, parent_id)) { // это родитель добавляемого узла
        mqn_connect(&COMPUTE_NODE, node_id, node_main_address, node_ping_address);
        mqn_reply(&COMPUTE_NODE, "OK");
    }
    free(node_id);
    free(parent_id);
    free(node_main_address);
    free(node_ping_address);
}
void cmd_remove(char* message) {
    char *node_id = NULL;
    sscanf(message, "remove %ms", &node_id);
    if (!strcmp(node_id, NODE_ID)) { // это удаляемый узел
        mqn_reply(&COMPUTE_NODE, "OK");
        mqn_destroy(&COMPUTE_NODE);
        exit(0);
    } else if (!strcmp(COMPUTE_NODE.left_child_id, node_id)) { // это левый сын
        char* answer = mqn_left_push(&COMPUTE_NODE, message);
        mqn_reply(&COMPUTE_NODE, answer);
    } else if (!strcmp(COMPUTE_NODE.right_child_id, node_id)) { // это правый сын
        char* answer = mqn_right_push(&COMPUTE_NODE, message);
        mqn_reply(&COMPUTE_NODE, answer);
    } else { // отправляем команду дальше по дереву
        char* answer = mqn_all_push(&COMPUTE_NODE, message);
        (!answer)? mqn_reply(&COMPUTE_NODE, ERROR_NODE_NOT_FOUND) : mqn_reply(&COMPUTE_NODE, answer);
    }
    free(node_id);
}
void cmd_exec(char* message) {
    
}
void cmd_ping(char* message) {
    char *node_id = NULL;
    sscanf(message, "ping %ms", &node_id);
    if (!strcmp(node_id, NODE_ID)) {
        mqn_reply(&COMPUTE_NODE, "OK");
    } else {
        char* answer = mqn_all_push(&COMPUTE_NODE, message);
        (!answer)? mqn_reply(&COMPUTE_NODE, ERROR_NODE_NOT_FOUND) : mqn_reply(&COMPUTE_NODE, answer);
    }
    free(node_id);
}
void cmd_disconnect(char* message) {
    char *node_id = NULL;
    sscanf(message, "disconnect %ms", &node_id);
    if (!strcmp(node_id, NODE_ID)) { // это удаляемый узел
        mqn_reply(&COMPUTE_NODE, ERROR_NODE_NOT_FOUND);
    } else if (!strcmp(COMPUTE_NODE.left_child_id, node_id) ||
                !strcmp(COMPUTE_NODE.right_child_id, node_id)) { // это сын
        mqn_close(&COMPUTE_NODE, node_id);
        mqn_reply(&COMPUTE_NODE, "OK");
    } else { // отправляем команду дальше по дереву
        char* answer = mqn_all_push(&COMPUTE_NODE, message);
        (!answer)? mqn_reply(&COMPUTE_NODE, ERROR_NODE_NOT_FOUND) : mqn_reply(&COMPUTE_NODE, answer);
    }
    free(node_id);
}