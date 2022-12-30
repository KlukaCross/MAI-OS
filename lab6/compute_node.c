#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "structures/mq_node.h"
#include "errors/error_handler.h"
#include "hashmap/include/hashmap.h"

void cmd_connect(char* message[], int size_message); // connect node_id parent_id main_address ping_address
void cmd_remove(char* message[], int size_message); // remove node_id
void cmd_exec(char* message[], int size_message); // exec node_id name value
void cmd_ping(char* message[], int size_message); // ping node_id
void cmd_disconnect(char* message[], int size_message); // disconnect main_address

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
    char* ping_address = argv[3];
    hashmap_init(&CHI_MAP, hashmap_hash_string, strcmp);

    error_handler_zmq(mqn_init(&COMPUTE_NODE));
    error_handler_zmq(mqn_bind(&COMPUTE_NODE, main_address, ping_address));

    char* buffer[10];
    int size;
    while (1) {
        error_handler_zmq((size = mqn_receive(COMPUTE_NODE.main_socket, buffer)) != -1);
        if (size > 0) {
            printf("command %s\n", buffer[1]);
            fflush(stdout);
            if (!strncmp(buffer[1], "exec", 4))
                cmd_exec(buffer, size);
            else if (!strncmp(buffer[1], "ping", 4))
                cmd_ping(buffer, size);
            else if (!strncmp(buffer[1], "remove", 6))
                cmd_remove(buffer, size);
            else if (!strncmp(buffer[1], "connect", 7) || !strncmp(buffer[1], "create", 6))
                cmd_connect(buffer, size);
            else if (!strncmp(buffer[1], "disconnect", 10))
                cmd_disconnect(buffer, size);
        }
        error_handler_zmq((size = mqn_receive(COMPUTE_NODE.left_child_main_socket, buffer)) != -1);
        if (size > 0)
            mqn_reply(&COMPUTE_NODE, (const char **) buffer);
        error_handler_zmq((size = mqn_receive(COMPUTE_NODE.right_child_main_socket, buffer)) != -1);
        if (size > 0)
            mqn_reply(&COMPUTE_NODE, (const char **) buffer);

        error_handler_zmq((size = mqn_receive(COMPUTE_NODE.ping_socket, buffer)) != -1);
        if (size > 0) {
            char* pong_msg[3] = {"pong", NODE_ID, NULL};
            mqn_reply(&COMPUTE_NODE, (const char **) pong_msg);
            if (COMPUTE_NODE.left_child_ping_socket) mqn_push(COMPUTE_NODE.left_child_ping_socket,
                                                              (const char **) buffer);
            if (COMPUTE_NODE.right_child_ping_socket) mqn_push(COMPUTE_NODE.right_child_ping_socket,
                                                               (const char **) buffer);
        }
        error_handler_zmq((size = mqn_receive(COMPUTE_NODE.left_child_ping_socket, buffer)) != -1);
        if (size > 0)
            mqn_reply(&COMPUTE_NODE, (const char **) buffer);
        error_handler_zmq((size = mqn_receive(COMPUTE_NODE.right_child_ping_socket, buffer) != -1));
        if (size > 0)
            mqn_reply(&COMPUTE_NODE, (const char **) buffer);
    }
    return 0;
}

void reply_not_found(char* uuid, char* command, char* node_id) {
    char code[12];
    sprintf(code, "%d", CODE_ERROR_NOT_FOUND);
    char* mes[6] = {uuid, command, node_id, code, ERROR_NOT_FOUND, NULL};
    mqn_reply(&COMPUTE_NODE, (const char **) mes);
}

void reply_ok(char* uuid, char* command, char* node_id) {
    char code[12];
    sprintf(code, "%d", CODE_OK);
    char* mes[6] = {uuid, command, node_id, code, "OK", NULL};
    mqn_reply(&COMPUTE_NODE, (const char **) mes);
}

void push_next(char* message[]) {
    if (COMPUTE_NODE.left_child_main_socket || COMPUTE_NODE.right_child_main_socket)
        mqn_push_all(&COMPUTE_NODE, (const char **) message);
    else
        reply_not_found(message[0], message[1], message[2]);
}

void cmd_connect(char* message[], int size_message) {
    char *uuid = message[0];
    char *command = message[1];
    char *node_id = message[2];
    char *parent_id = message[3];
    char *node_main_address = message[4];
    char *node_ping_address = message[5];
    if (strcmp(NODE_ID, parent_id) != 0 && strcmp(NODE_ID, node_id) != 0) { // отправляем дальше по узлам
        if (COMPUTE_NODE.left_child_main_socket || COMPUTE_NODE.right_child_main_socket)
            mqn_push_all(&COMPUTE_NODE, (const char **) message);
        else
            reply_not_found(uuid, command, node_id);
    } else if (!strcmp(NODE_ID, parent_id)) { // это родитель узла
        int int_id = strtol(node_id, NULL, 10);
        mqn_connect(&COMPUTE_NODE, int_id, node_main_address, node_ping_address);
        reply_ok(uuid, command, node_id);
    }
}
void cmd_remove(char* message[], int size_message) {
    char *uuid = message[0];
    char *command = message[1];
    char *node_id = message[2];
    if (!strcmp(node_id, NODE_ID)) { // это удаляемый узел
        reply_ok(uuid, command, node_id);
        mqn_destroy(&COMPUTE_NODE);
        exit(0);
    } else  // отправляем команду дальше по дереву
        push_next(message);
}
void cmd_exec(char* message[], int size_message) {
    char *uuid = message[0];
    char *command = message[1];
    char *node_id = message[2];
    char *name = message[3];
    if (!strcmp(node_id, NODE_ID)) {
        char code[12];
        if (size_message == 4) { // get
            int *res = hashmap_get(&CHI_MAP, name);
            if (!res) {
                char answer[strlen(name)+strlen("not found")+2];
                sprintf(code, "%d", CODE_ERROR_CUSTOM);
                strcat(answer, name);
                strcat(answer, " not found");
                char* mes[6] = {uuid, command, node_id, code, answer, NULL};
                mqn_reply(&COMPUTE_NODE, (const char **) mes);
            } else {
                char answer[12];
                sprintf(code, "%d", CODE_OK);
                sprintf(answer, "%d", *res);
                char* mes[6] = {uuid, command, node_id, code, answer, NULL};
                mqn_reply(&COMPUTE_NODE, (const char **) mes);
            }
        }
        else { // set
            char *value = message[4];
            int res = hashmap_put(&CHI_MAP, name, strtol(value, NULL, 10));
            if (res == 0) {
                char* answer = "unknown error";
                sprintf(code, "%d", CODE_ERROR_CUSTOM);
                char* mes[6] = {uuid, command, node_id, code, answer, NULL};
                mqn_reply(&COMPUTE_NODE, (const char **) mes);
            } else {
                sprintf(code, "%d", CODE_OK);
                char* mes[6] = {uuid, command, node_id, code, "", NULL};
                mqn_reply(&COMPUTE_NODE, (const char **) mes);
            }
        }
    } else
        push_next(message);
}
void cmd_ping(char* message[], int size_message) {
    char *uuid = message[0];
    char *command = message[1];
    char *node_id = message[2];
    if (!strcmp(node_id, NODE_ID)) {
        reply_ok(uuid, command, node_id);
    } else
        push_next(message);
}
void cmd_disconnect(char* message[], int size_message) {
    char *uuid = message[0];
    char *command = message[1];
    char *node_id = message[2];
    int int_node_id = strtol(node_id, NULL, 10);
    if (COMPUTE_NODE.left_child_id == int_node_id ||
                COMPUTE_NODE.right_child_id == int_node_id) { // это сын
        mqn_close(&COMPUTE_NODE, int_node_id);
        reply_ok(uuid, command, node_id);
    } else
        push_next(message);
}