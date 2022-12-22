#ifndef LAB6_MQ_NODE_H
#define LAB6_MQ_NODE_H

#include <zmq.h>
#include <stdbool.h>

typedef struct {
    void *context;
    void *socket;
    char* left_child_address;
    char* right_child_address;
    void* left_child_socket;
    void* right_child_socket;
} mq_node;

bool mqn_init(mq_node *m);
bool mqn_bind(mq_node *m, char* address);
bool mqn_connect(mq_node *m, char* address);

void mqn_receive(mq_node *m, char** message); // from parent
char* mqn_left_request(mq_node *m, char* message); // from left child
char* mqn_right_request(mq_node *m, char* message); // from right child
void mqn_send(mq_node *m, char* message); // to parent

void mqn_close(mq_node *m, char* address);
void mqn_destroy(mq_node *m);

#endif //LAB6_MQ_NODE_H
