#ifndef LAB6_MQ_NODE_H
#define LAB6_MQ_NODE_H

#include <zmq.h>
#include <stdbool.h>

typedef struct {
    void *context;
    void *main_socket;
    void *ping_socket;
    char* left_child_id;
    char* right_child_id;
    void* left_child_main_socket;
    void* right_child_main_socket;
    void* left_child_ping_socket;
    void* right_child_ping_socket;
} mq_node;

bool mqn_init(mq_node *m);
bool mqn_bind(mq_node *m, char* main_address, char* ping_address);
bool mqn_connect(mq_node *m, char* node_id, char* address, char* ping_address);

void mqn_receive(mq_node *m, zmq_msg_t* message); // from parent
char* mqn_left_push(mq_node *m, char* message); // to left child
char* mqn_right_push(mq_node *m, char* message); // to right child
char* mqn_all_push(mq_node *m, char* message); // to children
void mqn_reply(mq_node *m, const char* message); // to parent

void mqn_close(mq_node *m, char* node_id);
void mqn_destroy(mq_node *m);

#endif //LAB6_MQ_NODE_H
