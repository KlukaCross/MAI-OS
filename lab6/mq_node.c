#include "mq_node.h"
#include <stdlib.h>
#include <string.h>

bool mqn_init(mq_node *m) {
    if (!(m->context = zmq_ctx_new()))
        return false;
    if (!(m->main_socket = zmq_socket(m->context, ZMQ_REP)))
        return false;
    if (!(m->ping_socket = zmq_socket(m->context, ZMQ_REP)))
        return false;
    m->left_child_address = NULL;
    m->right_child_address = NULL;
    m->left_child_main_socket = NULL;
    m->right_child_main_socket = NULL;
    m->left_child_ping_socket = NULL;
    m->right_child_ping_socket = NULL;
    return true;
}
bool mqn_bind(mq_node *m, char* main_address, char* ping_adress) {
    return zmq_bind(m->context, main_address) == 0 && zmq_bind(m->context, ping_adress) == 0;
}
bool mqn_connect(mq_node *m, char* main_address, char* ping_address) {
    if (m->left_child_main_socket == NULL) {
        if (!(m->left_child_main_socket = zmq_socket(m->context, ZMQ_REQ)) ||
            !(m->left_child_ping_socket = zmq_socket(m->context, ZMQ_REQ)))
            return false;
        if (zmq_connect(m->left_child_main_socket, main_address) == -1 ||
            zmq_connect(m->left_child_ping_socket, ping_address) == -1)
            return false;
        m->left_child_address = main_address;
    } else if (m->right_child_main_socket == ZMQ_NULL) {
        if (!(m->right_child_main_socket = zmq_socket(m->context, ZMQ_REQ)) ||
            !(m->right_child_ping_socket = zmq_socket(m->context, ZMQ_REQ)))
            return false;
        if (zmq_connect(m->right_child_main_socket, main_address) == -1 ||
            zmq_connect(m->right_child_ping_socket, ping_address) == -1)
            return false;
        m->right_child_address = main_address;
    } else
        return false;
    return true;
}

void mqn_receive(mq_node *m, char** message) { // from parent
    zmq_msg_t req;
    zmq_msg_init(&req);
    zmq_msg_recv(&req, m->main_socket, 0);
    memcpy(*message, zmq_msg_data(&req), strlen(zmq_msg_data(&req)));
    zmq_msg_close(&req);
}

static char* mqn_push(void* socket, char* message) {
    zmq_msg_t req;
    zmq_msg_init_size(&req, strlen(message));
    memcpy(zmq_msg_data(&req), message, strlen(message));
    zmq_msg_send(&req, socket, 0);
    zmq_msg_close(&req);

    char *res = NULL;
    zmq_msg_t ans;
    zmq_msg_init(&ans);
    zmq_msg_recv(&ans, socket, 0); // timeout may be?
    memcpy(&res, zmq_msg_data(&ans), strlen(zmq_msg_data(&ans)));
    zmq_msg_close(&ans);
    return res;
}
char* mqn_left_push(mq_node *m, char* message) {
    if (m->left_child_main_socket != NULL)
        return mqn_push(m->left_child_main_socket, message);
    return NULL;
}
char* mqn_right_push(mq_node *m, char* message) {
    if (m->right_child_main_socket != NULL)
        return mqn_push(m->right_child_main_socket, message);
    return NULL;
}

void mqn_reply(mq_node *m, char* message) { // to parent
    zmq_msg_t reply;
    zmq_msg_init_size(&reply, strlen(message));
    memcpy(zmq_msg_data(&reply), message, strlen(message));
    zmq_msg_send(&reply, m->main_socket, 0);
    zmq_close(&reply);
}

void mqn_close(mq_node *m, char* address) {
    if (!strcmp(m->left_child_address, address)) {
        zmq_close(m->left_child_main_socket);
        zmq_close(m->left_child_ping_socket);
        m->left_child_main_socket = NULL;
        m->left_child_ping_socket = NULL;
        free(m->left_child_address);
        m->left_child_address = NULL;
    }
    else if (!strcmp(m->left_child_address, address)) {
        zmq_close(m->right_child_main_socket);
        zmq_close(m->right_child_ping_socket);
        m->right_child_main_socket = NULL;
        m->right_child_ping_socket = NULL;
        free(m->right_child_address);
        m->right_child_address = NULL;
    }
}
void mqn_destroy(mq_node *m) {
    if (m->left_child_address)
        mqn_close(m, m->left_child_address);
    if (m->right_child_address)
        mqn_close(m, m->right_child_address);
    zmq_close(m->main_socket);
    zmq_close(m->ping_socket);
    zmq_ctx_destroy (m->context);
    m->main_socket = NULL;
    m->ping_socket = NULL;
    m->context = NULL;
}
