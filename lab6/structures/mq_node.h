#ifndef LAB6_MQ_NODE_H
#define LAB6_MQ_NODE_H

#include <zmq.h>
#include <stdbool.h>

typedef struct {
    void *context;
    void *main_socket;
    void *ping_socket;
    int left_child_id;
    int right_child_id;
    void* left_child_main_socket;
    void* right_child_main_socket;
    void* left_child_ping_socket;
    void* right_child_ping_socket;
} mq_node;

void mqn_init(mq_node *m);
void mqn_bind(mq_node *m, char* main_address, char* ping_address);
void mqn_connect(mq_node *m, int node_id, char* address, char* ping_address);

int mqn_receive(void* socket, char* message[]);
void mqn_push(void* socket, char* message[]);
void mqn_push_all(mq_node *m, char* message[]);
void mqn_reply(mq_node *m, char* message[]);

void mqn_close(mq_node *m, int node_id);
void mqn_destroy(mq_node *m);

#endif //LAB6_MQ_NODE_H
