#include "mq_node.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

//#define DEBUG

void mqn_init(mq_node *m) {
    assert((m->context = zmq_ctx_new()));
    assert((m->main_socket = zmq_socket(m->context, ZMQ_PAIR)));
    assert((m->ping_socket = zmq_socket(m->context, ZMQ_PAIR)));
    m->left_child_id = -1;
    m->right_child_id = -1;
    m->left_child_main_socket = NULL;
    m->right_child_main_socket = NULL;
    m->left_child_ping_socket = NULL;
    m->right_child_ping_socket = NULL;
}
void mqn_bind(mq_node *m, char* main_address, char* ping_address) {
    assert(zmq_bind(m->main_socket, main_address) == 0);
    assert(zmq_bind(m->ping_socket, ping_address) == 0);
}
void mqn_connect(mq_node *m, int node_id, char* main_address, char* ping_address) {
    if (m->left_child_id == -1) {
        assert((m->left_child_main_socket = zmq_socket(m->context, ZMQ_PAIR)));
        assert((m->left_child_ping_socket = zmq_socket(m->context, ZMQ_PAIR)));
        assert(zmq_connect(m->left_child_main_socket, main_address) == 0);
        assert(zmq_connect(m->left_child_ping_socket, ping_address) == 0);
        m->left_child_id = node_id;
    } else if (m->right_child_id == -1) {
        assert((m->right_child_main_socket = zmq_socket(m->context, ZMQ_PAIR)));
        assert((m->right_child_ping_socket = zmq_socket(m->context, ZMQ_PAIR)));
        assert(zmq_connect(m->right_child_main_socket, main_address) == 0);
        assert(zmq_connect(m->right_child_ping_socket, ping_address) == 0);
        m->right_child_id = node_id;
    }
}

int mqn_receive(void* socket, char* message[]) { // from parent
    int i = 0;
    int64_t more;
    size_t more_size = sizeof more;
    do {
        zmq_msg_t part;
        int rc = zmq_msg_init (&part);
        assert (rc == 0);
        rc = zmq_msg_recv (&part, socket, ZMQ_DONTWAIT);
        if (rc == -1)
            return i;
        message[i] = realloc(message[i], sizeof(char)*(zmq_msg_size(&part)+1));
        assert(message[i]);
        memcpy(message[i], zmq_msg_data(&part), zmq_msg_size(&part));
        message[i][zmq_msg_size(&part)] = '\0';
        rc = zmq_getsockopt (socket, ZMQ_RCVMORE, &more, &more_size);
        assert (rc == 0);
        zmq_msg_close (&part);
        i++;
    } while (more);
    return i;
}

void mqn_push(void* socket, char* message[]) {
    int i=0;
    for (; message[i+1]; ++i) {
#ifdef DEBUG
        printf("send part %s...\n", message[i]);
        fflush(stdout);
#endif
        zmq_msg_t part;
        assert(zmq_msg_init_size(&part, strlen(message[i])) != -1);
        memcpy(zmq_msg_data(&part), message[i], strlen(message[i]));
        int rc = zmq_msg_send(&part, socket, ZMQ_SNDMORE);
#ifdef DEBUG
        if (errno)
            printf("zmq error: %s\n", zmq_strerror(errno));
        fflush(stdout);
#endif
        assert(rc != -1);
        assert(zmq_msg_close(&part) != -1);
#ifdef DEBUG
        printf("send part %s complete!\n", message[i]);
        fflush(stdout);
#endif
    }
#ifdef DEBUG
    printf("send part %s...\n", message[i]);
    fflush(stdout);
#endif
    zmq_msg_t part;
    assert(zmq_msg_init_size(&part, strlen(message[i])) != -1);
    memcpy(zmq_msg_data(&part), message[i], strlen(message[i]));
    int rc = zmq_msg_send(&part, socket, 0);
    assert(rc != -1);
    assert(zmq_msg_close(&part) != -1);
#ifdef DEBUG
    printf("send part %s complete!\n", message[i]);
    fflush(stdout);
#endif
}

void mqn_push_all(mq_node *m, char* message[]) {
    if (m->left_child_main_socket != NULL)
        mqn_push(m->left_child_main_socket, message);
    if (m->right_child_main_socket != NULL)
        mqn_push(m->right_child_main_socket, message);
}
void mqn_reply(mq_node *m, char* message[]) {
    mqn_push(m->main_socket, message);
}

void mqn_close(mq_node *m, int node_id) {
    if (m->left_child_id == node_id) {
        assert(zmq_close(m->left_child_main_socket) == 0);
        assert(zmq_close(m->left_child_ping_socket) == 0);
        m->left_child_main_socket = NULL;
        m->left_child_ping_socket = NULL;
        m->left_child_id = -1;
    }
    else if (m->right_child_id == node_id) {
        assert(zmq_close(m->right_child_main_socket) == 0);
        assert(zmq_close(m->right_child_ping_socket) == 0 );
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
    assert(zmq_close(m->main_socket) == 0);
    assert(zmq_close(m->ping_socket) == 0);
    assert(zmq_ctx_destroy (m->context) == 0);
    m->main_socket = NULL;
    m->ping_socket = NULL;
    m->context = NULL;
}
