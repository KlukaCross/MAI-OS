#include "mq_node.h"
#include <string.h>

bool mqn_init(mq_node *m) {
    if (!(m->context = zmq_ctx_new()))
        return false;
    if (!(m->main_socket = zmq_socket(m->context, ZMQ_REP)))
        return false;
    if (!(m->ping_socket = zmq_socket(m->context, ZMQ_REP)))
        return false;
    m->left_child_id = -1;
    m->right_child_id = -1;
    m->left_child_main_socket = NULL;
    m->right_child_main_socket = NULL;
    m->left_child_ping_socket = NULL;
    m->right_child_ping_socket = NULL;
    return true;
}
bool mqn_bind(mq_node *m, char* main_address, char* ping_address) {
    return zmq_bind(m->main_socket, main_address) == 0 && zmq_bind(m->ping_socket, ping_address) == 0;
}
bool mqn_connect(mq_node *m, int node_id, char* main_address, char* ping_address) {
    if (m->left_child_id == -1) {
        if (!(m->left_child_main_socket = zmq_socket(m->context, ZMQ_REQ)) ||
            !(m->left_child_ping_socket = zmq_socket(m->context, ZMQ_REQ)))
            return false;
        if (zmq_connect(m->left_child_main_socket, main_address) == -1 ||
            zmq_connect(m->left_child_ping_socket, ping_address) == -1)
            return false;
        m->left_child_id = node_id;
    } else if (m->right_child_id == -1) {
        if (!(m->right_child_main_socket = zmq_socket(m->context, ZMQ_REQ)) ||
            !(m->right_child_ping_socket = zmq_socket(m->context, ZMQ_REQ)))
            return false;
        if (zmq_connect(m->right_child_main_socket, main_address) == -1 ||
            zmq_connect(m->right_child_ping_socket, ping_address) == -1)
            return false;
        m->right_child_id = node_id;
    } else
        return false;
    return true;
}

int mqn_receive(void* socket, char* message[]) { // from parent
    int i=0;
    while (1) {
        zmq_msg_t msg;
        if (zmq_msg_init(&msg) == -1) return -1;
        int size = zmq_msg_recv(&msg, socket, ZMQ_DONTWAIT);
        if (size == -1)
            return -1;
        memcpy(message[i], zmq_msg_data(&msg), strlen(zmq_msg_data(&msg)));
        if (zmq_msg_close(&msg) == -1) return -1;
        i++;
        if (!zmq_msg_more(&msg))
            break;
    }
    return i;
}

bool mqn_push(void* socket, const char* message[]) {
    int i=0;
    for (; message[i+1]; ++i) {
        zmq_msg_t msg;
        if (zmq_msg_init_size(&msg, strlen(message[i])) == -1) return false;
        memcpy(zmq_msg_data(&msg), message[i], strlen(message[i]));
        if (zmq_msg_send(&msg, socket, ZMQ_SNDMORE) == -1) return false;
        if (zmq_msg_close(&msg) == -1) return false;
    }
    zmq_msg_t msg;
    if (zmq_msg_init_size(&msg, strlen(message[i])) == -1) return false;
    memcpy(zmq_msg_data(&msg), message[i], strlen(message[i]));
    if (zmq_msg_send(&msg, socket, 0) == -1) return false;
    if (zmq_msg_close(&msg) == -1) return false;
    return true;
}
bool mqn_push_all(mq_node *m, const char* message[]) {
    if (m->left_child_main_socket != NULL)
        if (!mqn_push(m->left_child_main_socket, message)) return false;
    if (m->right_child_main_socket != NULL)
        if (!mqn_push(m->right_child_main_socket, message)) return false;
    return true;
}
bool mqn_reply(mq_node *m, const char* message[]) {
    return mqn_push(m->main_socket, message);
}

void mqn_close(mq_node *m, int node_id) {
    if (m->left_child_id == node_id) {
        zmq_close(m->left_child_main_socket);
        zmq_close(m->left_child_ping_socket);
        m->left_child_main_socket = NULL;
        m->left_child_ping_socket = NULL;
        m->left_child_id = -1;
    }
    else if (m->right_child_id == node_id) {
        zmq_close(m->right_child_main_socket);
        zmq_close(m->right_child_ping_socket);
        m->right_child_main_socket = NULL;
        m->right_child_ping_socket = NULL;
        m->right_child_id = -1;
    }
}
void mqn_destroy(mq_node *m) {
    if (m->left_child_id != -1)
        mqn_close(m, m->left_child_id);
    if (m->right_child_id != -1)
        mqn_close(m, m->right_child_id);
    zmq_close(m->main_socket);
    zmq_close(m->ping_socket);
    zmq_ctx_destroy (m->context);
    m->main_socket = NULL;
    m->ping_socket = NULL;
    m->context = NULL;
}
