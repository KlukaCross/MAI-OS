#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "balanced_tree.h"
#include "mq_node.h"
#include "error_handler.h"
#include "d_string.h"

char* gen_address();

void cmd_create(char* message); // create id
void cmd_remove(char* message); // remove id
void cmd_exec(char* message); // exec id name value
void cmd_heartbeat(char* message); // heartbeat time

bool ping(char* node_id);
bool disconnect(char* address);
bool connect(char* node_id, char* main_address, char* ping_address);

int address_shift;
b_tree TREE;
mq_node CONTROL_NODE;
b_tree_node C_NODE;

int main (void)
{
    btree_init(&TREE);
    C_NODE.id = "-1";
    C_NODE.pid = getpid();
    C_NODE.is_available = true;
    btree_add(&TREE, C_NODE);

    address_shift = 0;
    char* main_address = gen_address(&address_shift);

    error_handler(mqn_init(&CONTROL_NODE));
    error_handler(mqn_bind(&CONTROL_NODE, main_address));

    char* buffer = NULL;
    int* buffer_len = NULL;
    while (1) {
        scan_string(&buffer, buffer_len);
        if (!(strncmp(buffer, "exec", 4)))
            cmd_exec(buffer);
        if (!(strncmp(buffer, "create", 6)))
            cmd_create(buffer);
        if (!(strncmp(buffer, "remove", 6)))
            cmd_remove(buffer);
        if (!(strncmp(buffer, "heartbit", 8)))
            cmd_heartbeat(buffer);
        free(buffer);
    }
    return 0;
}

char* gen_address() {
    char buff[12];
    char* res;
    sprintf(buff, "%d", address_shift++);
    res = strcat("tcp://*:", buff);
    return res;
}

void cmd_create(char* message) {
    char* node_id;
    sscanf(message, "create %ms", &node_id);
    if (btree_find(&TREE, node_id).pid != -1) { // если такой узел уже есть
        printf("Error: Already exists");
        return;
    }

    char* main_address = gen_address();
    char* ping_address = gen_address();

    int pid = fork();
    if (pid < 0) {
        perror("node create error: ");
        exit(errno);
    } else if (pid == 0) {
        execl("./COMPUTE_NODE", "./COMPUTE_NODE", node_id, main_address, ping_address, (char *) NULL);
        perror("node create error: ");
        exit(errno);
    } else {
        b_tree_node node = {node_id, pid, main_address, ping_address, true};
        btree_add(&TREE, node);
        b_tree_node new_node_parent = btree_last_elem_parent(&TREE);
        if (!strcmp(new_node_parent.id, C_NODE.id)) { // если родитель нового узла - это управляющий
            if (!mqn_connect(&CONTROL_NODE, main_address, ping_address))
                printf("Create node error\n");
        }
        else {
            char push_message[strlen("create")+strlen(node_id)+
            strlen(new_node_parent.id)+strlen(main_address)+strlen(ping_address)+5];
            sprintf(push_message, "create %s %s %s %s", node_id, new_node_parent.id, main_address, ping_address);
            char *answer = mqn_left_push(&CONTROL_NODE, push_message);
            if (parent_not_found(answer)) // если не нашли родителя в левом поддереве, ищем в правом
                answer = mqn_right_push(&CONTROL_NODE, push_message);
            if (!is_ok(push_message)) { // если ошибка, то удаляем новый узел
                btree_remove(&TREE, node_id);
                kill(pid, SIGKILL);
            }
            if (parent_not_found(answer)) // если не нашли (либо просто не связались) родителя в правом поддерере
                printf("Error: failed to create node because parent %s is unavailable\n", new_node_parent.id);
            else
                printf("%s\n", answer);
        }
    }
    free(node_id);
}

void cmd_remove(char* message) {
    char* node_id;
    sscanf(message, "create %ms", &node_id);
    b_tree_node node = btree_find(&TREE, node_id);
    if (node.pid == -1) { // если такого узла нет
        printf("Error: Not found\n");
        free(node_id);
        return;
    } else if (node.pid == C_NODE.pid) { // если удаляемый узел - это управляющий
        printf("Error: The control node cannot be deleted\n");
        free(node_id);
        return;
    }

    if (!ping(node_id)) {  // удаляемый узел не отвечает (обиделся)
        printf("Error: Node is unavailable\n");
        free(node_id);
        return;
    }

    b_tree_node last_node = btree_last_elem(&TREE);
    if (last_node.id != node_id) { // если удаляемый узел не является последним, пингуем последнего
        if (!ping(last_node.id)) {
            printf("Balancing error: The last node is unavailable\n");
            free(node_id);
            return;
        }
    }

    if (!disconnect(last_node.id)) { // отключаем последний узел от его родителя
        printf("Balancing Error: Failed to disconnect the last node\n");
        free(node_id);
        return;
    }

    // сыновья удаляемого узла
    b_tree_node left_child_of_removed_node = btree_get_left(&TREE, node_id);
    b_tree_node right_child_of_removed_node = btree_get_right(&TREE, node_id);
    // родитель удаляемого узла
    b_tree_node parent_of_removed_node = btree_get_parent(&TREE, node_id);

    char push_message[strlen("remove")+strlen(node_id)+2];
    sprintf(push_message, "remove %s", node_id);
    char *answer = mqn_left_push(&CONTROL_NODE, push_message);
    if (parent_not_found(answer)) // если не нашли узел в левом поддереве, ищем в правом
        answer = mqn_right_push(&CONTROL_NODE, push_message);
    if (!is_ok(push_message)) { // если ошибка, то не удаляем новый узел и подключаем обратно последний узел
        printf("%s\n", answer);
        b_tree_node last_node_parent = btree_last_elem_parent(&TREE);
        connect(last_node_parent.id, last_node.main_address, last_node.ping_address);
        free(node_id);
        return;
    }
    kill(node.pid, SIGKILL);

    // балансим дерево TREE и дерево в сети
    btree_remove(&TREE, node_id);
    if (!connect(parent_of_removed_node.id, last_node.main_address, last_node.ping_address))
        printf("Balancing Error: Failed to connect node %s to node %s\n", parent_of_removed_node.id, last_node.id);
    if (!connect(last_node.id, left_child_of_removed_node.main_address, left_child_of_removed_node.ping_address))
        printf("Balancing Error: Failed to connect node %s to node %s\n", last_node.id, left_child_of_removed_node.id);
    if (!connect(last_node.id, right_child_of_removed_node.main_address, right_child_of_removed_node.ping_address))
        printf("Balancing Error: Failed to connect node %s to node %s\n", last_node.id, right_child_of_removed_node.id);

    printf("%s\n", answer);
    free(node_id);
}

void cmd_exec(char* message); // exec id name value
void cmd_heartbeat(char* message); // heartbeat time


bool ping(char* node_id) {
    char push_message[strlen("ping")+strlen(node_id)+2];
    sprintf(push_message, "ping %s", node_id);
    char *answer = mqn_left_push(&CONTROL_NODE, push_message);
    if (node_not_found(answer)) // если не нашли узел в левом поддереве, ищем в правом
        answer = mqn_right_push(&CONTROL_NODE, push_message);
    return is_ok(answer);
}
bool disconnect(char* address) {
    char push_message[strlen("disconnect")+strlen(address)+2];
    sprintf(push_message, "disconnect %s", address);
    char *answer = mqn_left_push(&CONTROL_NODE, push_message);
    if (node_not_found(answer)) // если не нашли узел в левом поддереве, ищем в правом
        answer = mqn_right_push(&CONTROL_NODE, push_message);
    return is_ok(answer);
}
bool connect(char* node_id, char* main_address, char* ping_address) {
    char push_message[strlen("connect")+strlen(node_id)+strlen(main_address)+strlen(ping_address)+4];
    sprintf(push_message, "connect %s %s %s", node_id, main_address, ping_address);
    char *answer = mqn_left_push(&CONTROL_NODE, push_message);
    if (parent_not_found(answer)) // если не нашли узел в левом поддереве, ищем в правом
        answer = mqn_right_push(&CONTROL_NODE, push_message);
    return is_ok(answer);
}