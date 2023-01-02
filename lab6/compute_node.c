#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include "structures/mq_node.h"
#include "errors/error_handler.h"
#include "hashmap/include/hashmap.h"

#define MILLISECONDS_SLEEP 50
#define SECONDS_SLEEP 0

//#define DEBUG

void cmd_connect(char* message[], int size_message); // connect node_id parent_id main_address ping_address
void cmd_remove(char* message[], int size_message); // remove node_id
void cmd_exec(char* message[], int size_message); // exec node_id name value
void cmd_ping(char* message[], int size_message); // ping node_id
void cmd_disconnect(char* message[], int size_message); // disconnect main_address

char* NODE_ID;
mq_node COMPUTE_NODE;

HASHMAP(char, int) CHI_MAP;

const struct timespec SLEEP_TIME = {SECONDS_SLEEP, MILLISECONDS_SLEEP*1000000L}; // sec nanosec

void clean_buffer(char** buffer) {
    for (int i = 0; buffer[i]; ++i) {
        free(buffer[i]);
        buffer[i] = NULL;
    }
}

// prog id main_address ping_address
int main(int argc, char** argv){
    if (argc != 4) {
        printf("compute node initial error: invalid number of arguments\n");
        exit(1);
    }
    NODE_ID = argv[1];
    char* main_address = argv[2];
    char* ping_address = argv[3];
    hashmap_init(&CHI_MAP, hashmap_hash_string, strcmp);

    mqn_init(&COMPUTE_NODE);
    mqn_bind(&COMPUTE_NODE, main_address, ping_address);
#ifdef DEBUG
    printf("node %s with main_address=%s and ping_address=%s is ready\n", NODE_ID, main_address, ping_address);
#endif
    char* buffer[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    int size;
    while (1) {
        size = mqn_receive(COMPUTE_NODE.main_socket, buffer);
        if (size > 0) {
            char* command = buffer[1];
#ifdef DEBUG
            printf("node %s receive command %s from parent\n", NODE_ID, command);
            fflush(stdout);
#endif
            if (!strncmp(command, "exec", 4))
                cmd_exec(buffer, size);
            else if (!strncmp(command, "ping", 4))
                cmd_ping(buffer, size);
            else if (!strncmp(command, "remove", 6))
                cmd_remove(buffer, size);
            else if (!strncmp(command, "connect", 7) || !strncmp(command, "create", 6))
                cmd_connect(buffer, size);
            else if (!strncmp(command, "disconnect", 10))
                cmd_disconnect(buffer, size);
            clean_buffer(buffer);
        }
        size = mqn_receive(COMPUTE_NODE.left_child_main_socket, buffer);
        if (size > 0) {
#ifdef DEBUG
            printf("node %s receive command %s %s from node %d\n", NODE_ID, reply_buffer[1], reply_buffer[2], COMPUTE_NODE.left_child_id);
            if (reply_buffer[5])
                printf("ERROR in node %s: last element of reply_buffer must be NULL\n", NODE_ID);
            fflush(stdout);
#endif
            mqn_reply(&COMPUTE_NODE, buffer);
            clean_buffer(buffer);
#ifdef DEBUG
            printf("node %s reply %s for command %s\n", NODE_ID, reply_buffer[4], reply_buffer[1]);
            fflush(stdout);
#endif
        }
        size = mqn_receive(COMPUTE_NODE.right_child_main_socket, buffer);
        if (size > 0) {
#ifdef DEBUG
            printf("node %s receive command %s from node %d\n", NODE_ID, reply_buffer[1], COMPUTE_NODE.right_child_id);
            if (reply_buffer[5])
                printf("ERROR in node %s: last element of reply_buffer must be NULL\n", NODE_ID);
            fflush(stdout);
#endif
            mqn_reply(&COMPUTE_NODE, buffer);
            clean_buffer(buffer);
#ifdef DEBUG
            printf("node %s reply %s for command %s\n", NODE_ID, reply_buffer[4], reply_buffer[1]);
            fflush(stdout);
#endif
        }
        size = mqn_receive(COMPUTE_NODE.ping_socket, buffer);
        if (size > 0) {
            char* pong_msg[3] = {"pong", NODE_ID, NULL};
            mqn_push(COMPUTE_NODE.ping_socket, pong_msg);
            if (COMPUTE_NODE.left_child_ping_socket) mqn_push(COMPUTE_NODE.left_child_ping_socket, buffer);
            if (COMPUTE_NODE.right_child_ping_socket) mqn_push(COMPUTE_NODE.right_child_ping_socket, buffer);
            clean_buffer(buffer);
        }
        size = mqn_receive(COMPUTE_NODE.left_child_ping_socket, buffer);
        if (size > 0) {
            mqn_push(COMPUTE_NODE.ping_socket, buffer);
            clean_buffer(buffer);
        }
        size = mqn_receive(COMPUTE_NODE.right_child_ping_socket, buffer);
        if (size > 0) {
            mqn_push(COMPUTE_NODE.ping_socket, buffer);
            clean_buffer(buffer);
        }
        nanosleep(&SLEEP_TIME, NULL);
    }
    return 0;
}

char* int_to_str(int a) {
    char* res = malloc(sizeof(char)*12);
    sprintf(res, "%d", a);
    return res;
}

void reply_not_found(char* uuid, char* command, char* node_id) {
    char *code = int_to_str(CODE_ERROR_NOT_FOUND);
    char* mes[6] = {uuid, command, node_id, code, ERROR_NOT_FOUND, NULL};
#ifdef DEBUG
    printf("node %s reply NOT FOUND for command %s\n", NODE_ID, command);
    fflush(stdout);
#endif
    mqn_reply(&COMPUTE_NODE, mes);
    free(code);
}

void reply_ok(char* uuid, char* command, char* node_id) {
    char *code = int_to_str(CODE_OK);
    char* mes[6] = {uuid, command, node_id, code, "OK", NULL};
#ifdef DEBUG
    printf("node %s reply OK for command %s\n", NODE_ID, command);
    fflush(stdout);
#endif
    mqn_reply(&COMPUTE_NODE, mes);
    free(code);
}

void push_next(char* message[]) {
    if (COMPUTE_NODE.left_child_main_socket || COMPUTE_NODE.right_child_main_socket) {
        mqn_push_all(&COMPUTE_NODE, message);
#ifdef DEBUG
        printf("node %s push command %s to children (left child id = %d)(right child id = %d)\n", NODE_ID, message[1], COMPUTE_NODE.left_child_id, COMPUTE_NODE.right_child_id);
        fflush(stdout);
#endif
    }
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
    if (strcmp(NODE_ID, parent_id) != 0 && strcmp(NODE_ID, node_id) != 0) // отправляем дальше по узлам
        push_next(message);
    else if (!strcmp(NODE_ID, parent_id)) { // это родитель узла
        int int_id = strtol(node_id, NULL, 10);
        mqn_connect(&COMPUTE_NODE, int_id, node_main_address, node_ping_address);
#ifdef DEBUG
        printf("node %s connected node %s. \tLeft node: %d\t Right node %d\n", NODE_ID, node_id, COMPUTE_NODE.left_child_id, COMPUTE_NODE.right_child_id);
#endif
        reply_ok(uuid, command, node_id);
    }
}
void cmd_remove(char* message[], int size_message) {
    char *uuid = message[0];
    char *command = message[1];
    char *node_id = message[2];
    if (!strcmp(node_id, NODE_ID)) { // это удаляемый узел
        reply_ok(uuid, command, node_id);
#ifdef DEBUG
        printf("node %s is being destroyed...\n", NODE_ID);
        fflush(stdout);
#endif
        mqn_destroy(&COMPUTE_NODE);
#ifdef DEBUG
        printf("node %s was destroyed\n", NODE_ID);
        fflush(stdout);
#endif
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
        char *code;
        if (size_message == 4) { // get
            int *res = hashmap_get(&CHI_MAP, name);
            if (!res) {
                char answer[strlen(name)+strlen("not found")+2];
                answer[0] = '\0';
                code = int_to_str(CODE_ERROR_CUSTOM);
                strcat(answer, name);
                strcat(answer, " not found");
                char* mes[6] = {uuid, command, node_id, code, answer, NULL};
                mqn_reply(&COMPUTE_NODE, mes);
            } else {
                char *answer = int_to_str(*res);
                code = int_to_str(CODE_OK);
                char* mes[6] = {uuid, command, node_id, code, answer, NULL};
                mqn_reply(&COMPUTE_NODE, mes);
                free(answer);
            }
        }
        else { // set
            char *value = message[4];
            int *int_value = malloc(sizeof(int));
            *int_value = strtol(value, NULL, 10);
            char* k = malloc(strlen(name)+1);
            memcpy(k, name, strlen(name));
            k[strlen(name)] = '\0';
            int res = hashmap_put(&CHI_MAP, k, int_value);
            if (res) {
                char* answer = (res == -EEXIST)? "already exists" : "unknown error";
                code = int_to_str(CODE_ERROR_CUSTOM);
                char* mes[6] = {uuid, command, node_id, code, answer, NULL};
                mqn_reply(&COMPUTE_NODE, mes);
            } else {
                code = int_to_str(CODE_OK);
                char* mes[6] = {uuid, command, node_id, code, "", NULL};
                mqn_reply(&COMPUTE_NODE, mes);
            }
        }
        free(code);
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