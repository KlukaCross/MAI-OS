#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <uuid/uuid.h>
#include <time.h>
#include "structures/balanced_tree.h"
#include "structures/mq_node.h"
#include "errors/error_handler.h"
#include "hashmap/include//hashmap.h"
#include "structures/rm_list.h"

void print_help() {
    printf("USAGE:\n"
           "\tcreate ID - create node with id=ID\n"
           "\tremove ID - remove node with id=ID\n"
           "\texec ID NAME [VALUE] - get value or set VALUE bu the key NAME\n"
           "\theartbeat TIME - each node begins to report once every TIME milliseconds that it is operational. If "
           "there is no signal from the node for 4*TIME milliseconds, then a line about the node's unavailability is displayed\n");
}

void gen_address(char rep_address[32], char req_address[32]);
void gen_uuid(char uuid[16]);
void answer_count_push(char *uuid);

void cmd_create(int id); // create id
void cmd_remove(int id); // remove id
void cmd_exec(bool is_set, int id, char* name, int value); // exec id name value
bool cmd_heartbeat(int time); // heartbeat time

void rec_create(int id, int code, char* answer, int count_answers);
void rec_remove(int id, int code, char* answer, int count_answers);
void rec_exec(int id, int code, char* answer, int count_answers);
void rec_ping(int id, int code, char* answer, int count_answers);
void rec_connect(int id, int code, char* answer, int count_answers);
void rec_disconnect(int id, int code, char* answer, int count_answers);

void push_create(int id, int parent_id, char* main_address, char* ping_address);
void push_connect(int id, int parent_id, char* main_address, char* ping_address);
void push_disconnect(int id);
void push_ping(int id);
void push_remove(int id);

void *receive_worker(void* args);
void *heartbeat_worker(void* args);

int address_shift;
b_tree TREE;
mq_node CONTROL_NODE;
b_tree_node C_NODE;

int heartbeat_time = 0;

HASHMAP(char*, int) answer_count_map;
rm_list remove_list;

int main (void)
{
    btree_init(&TREE);
    C_NODE.id = -1;
    C_NODE.pid = getpid();
    btree_add(&TREE, C_NODE);
    hashmap_init(&answer_count_map, hashmap_hash_string, strcmp);
    list_init(&remove_list);

    address_shift = 5000;
    char main_address[32] = "";
    gen_address(main_address, NULL);
    char ping_address[32] = "";
    gen_address(ping_address, NULL);
    error_handler_zmq(mqn_init(&CONTROL_NODE));
    error_handler_zmq(mqn_bind(&CONTROL_NODE, main_address, ping_address));

    pthread_t receive_thread;
    pthread_t heartbeat_thread;

    error_handler(!pthread_create(&receive_thread, NULL, receive_worker, NULL), "create thread error");
    error_handler(!pthread_detach(receive_thread), "detach thread error");

    int size;
    int arg1, arg3;
    char *arg2 = NULL;
    bool res_cmd = false;
    print_help();
    while (true) {
        size = scanf("create %d", &arg1);
        if (size == EOF)
            break;
        if (size == 1)
            cmd_create(arg1);
        else if ((size = scanf("remove %d", &arg1)) == 1)
            cmd_remove(arg1);
        else if ((scanf("exec %d %ms", &arg1, &arg2)) == 2) {
            int c = getchar();
            if (c == ' ')
                size = scanf("%d", &arg3);
            (size == 1)? cmd_exec(true, arg1, arg2, arg3) : cmd_exec(false, arg1, arg2, arg3);
        } else if ((scanf("heartbeat %d", &arg1)) == 1) {
            res_cmd = cmd_heartbeat(arg1);
            if (res_cmd) {
                error_handler(!pthread_create(&heartbeat_thread, NULL, heartbeat_worker, NULL), "create thread error");
                error_handler(!pthread_detach(heartbeat_thread), "detach thread error");
            }
        } else {
            scanf("%ms", &arg2);
            printf("unknown command - %s\n", arg2);
            print_help();
        }
        getchar();
    }
    free(arg2);
    return 0;
}

void gen_address(char rep_address[32], char req_address[32]) {
    char buff[12];
    sprintf(buff, "%d", address_shift++);
    strcat(rep_address, "tcp://*:");
    strcat(rep_address, buff);
    if (req_address) {
        strcat(req_address, "tcp://localhost:");
        strcat(req_address, buff);
    }
}

void gen_uuid(char uuid[37]) {
    uuid_t uu;
    uuid_generate_random(uu);
    uuid_unparse(uu, uuid);
}

void answer_count_push(char *uuid) {
    hashmap_put(&answer_count_map, uuid, (TREE.size-1)/2);
}

void cmd_create(int id) {
    if (btree_find(&TREE, id).pid != -1) { // если такой узел уже есть
        printf("Error: Already exists\n");
        return;
    }

    char rep_main_address[32] = "";
    char req_main_address[32] = "";
    gen_address(rep_main_address, req_main_address);
    char rep_ping_address[32] = "";
    char req_ping_address[32] = "";
    gen_address(rep_ping_address, req_ping_address);

    int pid = fork();
    error_handler(pid >= 0, "node create error\n");
    char str_id[12];
    sprintf(str_id, "%d", id);
    if (pid == 0) {
        execl("./compute_node", "./compute_node", str_id, rep_main_address, rep_ping_address, (char *) NULL);
        perror("node create error\n");
        exit(errno);
    } else {
        b_tree_node node = {id, pid, req_main_address, req_ping_address};
        btree_add(&TREE, node);
        b_tree_node new_node_parent = btree_last_elem_parent(&TREE);
        push_create(id, new_node_parent.id, req_main_address, req_ping_address);
    }
}

void cmd_remove(int id) {
    b_tree_node removed_node = btree_find(&TREE, id);
    if (removed_node.pid == -1) { // если такого узла нет
        printf("Error: Not found\n");
        return;
    } else if (removed_node.pid == C_NODE.pid) { // если удаляемый узел - это управляющий
        printf("Error: The control removed_node cannot be deleted\n");
        return;
    }

    b_tree_node last_node = btree_last_elem(&TREE);
    b_tree_node left_child_of_removed_node = btree_get_left(&TREE, id);
    b_tree_node right_child_of_removed_node = btree_get_right(&TREE, id);
    b_tree_node parent_of_removed_node = btree_get_parent(&TREE, id);

    remove_struct rms = {removed_node, last_node, left_child_of_removed_node, right_child_of_removed_node, parent_of_removed_node};
    list_iter li = list_iter_begin(&remove_list);
    list_iter_insert_before(&li, rms);

    if (last_node.id == removed_node.id) // если удаляемый узел - это последний, то используем сразу команду remove
        push_remove(id);
    else
        // иначе: проверяем, доступен ли последний узел, затем отсоединяем его, удаляем удаляемый узел,
        // отcоединяем его родителя от него, подключаем родителя удаляемого узла к последнему, последний подключаем к сыновьям удаленного узла
        push_ping(last_node.id);
}

void cmd_exec(bool is_set, int id, char* name, int value) {
    b_tree_node removed_node = btree_find(&TREE, id);
    if (removed_node.pid == -1) { // если такого узла нет
        printf("Error: Not found\n");
        return;
    } else if (removed_node.pid == C_NODE.pid) { // если узел - это управляющий
        printf("Error: The control node cannot be computational\n");
        return;
    }

    char str_id[12];
    sprintf(str_id, "%d", id);
    char uuid[37];
    gen_uuid(uuid);
    if (is_set) {
        char str_value[12];
        sprintf(str_value, "%d", value);
        char* push_message[6] = {uuid, "exec", str_id, name, str_value, NULL};
        error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    } else {
        char* push_message[5] = {uuid, "exec", str_id, name, NULL};
        error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    }
    answer_count_push(uuid);
}
bool cmd_heartbeat(int time) {
    if (time < 0) {
        printf("Error: time must be equal or greater than zero\n");
        return false;
    }
    if (time == 0) {
        printf("Heartbeat stopped\n");
        heartbeat_time = time;
        return false;
    }
    printf("OK\n");
    if (heartbeat_time != 0) {
        heartbeat_time = time;
        return true;
    }
    heartbeat_time = time;
    return false;
}

void rec_create(int id, int code, char* answer, int count_answers) {
    if (code == CODE_ERROR_NOT_FOUND) {
        if (count_answers == 0)
            printf("Error:%d: Node not found\n", id);
        return;
    }
    b_tree_node created_node = btree_find(&TREE, id);
    if (code == CODE_ERROR_CUSTOM) {
        printf("Error: %s\n", answer);
        btree_remove(&TREE, id);
        kill(created_node.pid, SIGKILL);
    } else if (code == CODE_OK)
        printf("Ok: %d\n", created_node.pid);
}
void rec_remove(int id, int code, char* answer, int count_answers) {
    list_iter li = list_iter_begin(&remove_list);
    for (; list_iter_equal(li, list_iter_end(
            &remove_list)); list_iter_move_next(&li)) {
        remove_struct rs = list_iter_get(&li);
        if (rs.replacing_with_node.id == id)
            break;
    }
    if ((code == CODE_ERROR_NOT_FOUND && count_answers == 0) || code == CODE_ERROR_CUSTOM) {
        list_iter_remove(&li);
        printf("Balancing Error: Failed to disconnect the last node\n");
    } else if (code == CODE_OK) {
        remove_struct rs = list_iter_get(&li);
        btree_remove(&TREE, rs.removed_node.id);
        push_disconnect(rs.removed_node_parent.id);
    }
}
void rec_exec(int id, int code, char* answer, int count_answers) {
    if (code == CODE_ERROR_NOT_FOUND && count_answers == 0 || code == CODE_OK || code == CODE_ERROR_CUSTOM)
        printf("Error:%d: %s\n", id, answer);
}
void rec_ping(int id, int code, char* answer, int count_answers) {
    list_iter li = list_iter_begin(&remove_list);
    for (; list_iter_equal(li, list_iter_end(&remove_list)); list_iter_move_next(&li)) {
        remove_struct rs = list_iter_get(&li);
        if (rs.replacing_with_node.id == id)
            break;
    }
    if ((code == CODE_ERROR_NOT_FOUND && count_answers == 0) || code == CODE_ERROR_CUSTOM) {
        if (!list_iter_equal(li, list_iter_end(&remove_list))) { // если команда входит в цепочку удаления
            list_iter_remove(&li);
            printf("Balancing error: The last removed_node is unavailable\n");
        } else
            printf("Error: Not found\n");
    } else if (code == CODE_OK) {
        if (!list_iter_equal(li, list_iter_end(&remove_list))) // если команда входит в цепочку удаления
            push_disconnect(id);
        else
            printf("Ok\n");
    }
}
void rec_connect(int id, int code, char* answer, int count_answers) {
    list_iter li = list_iter_begin(&remove_list);
    for (; list_iter_equal(li, list_iter_end(
            &remove_list)); list_iter_move_next(&li)) {
        remove_struct rs = list_iter_get(&li);
        if (rs.replacing_with_node.id == id)
            break;
    }
    if ((code == CODE_ERROR_NOT_FOUND && count_answers == 0) || code == CODE_ERROR_CUSTOM) {
        if (!list_iter_equal(li, list_iter_end(&remove_list))) { // если команда входит в цепочку удаления
            remove_struct rs = list_iter_get(&li);
            list_iter_remove(&li);
            if (rs.replacing_with_node.id == id)
                printf("Balancing Error: Failed to connect node %d to node %d\n", rs.removed_node_parent.id, id);
            if (rs.removed_node_left_child.id == id || rs.removed_node_right_child.id == id)
                printf("Balancing Error: Failed to connect node %d to node %d\n", rs.replacing_with_node.id, id);
        } else
            printf("Error: Failed to connect\n");
    } else if (code == CODE_OK) {
        if (!list_iter_equal(li, list_iter_end(&remove_list))) { // если команда входит в цепочку удаления
            remove_struct rs = list_iter_get(&li);
            if (rs.removed_node_right_child.id == id ||
            (rs.removed_node_left_child.id == id && rs.removed_node_right_child.id == -1) ||
            (rs.removed_node_parent.id == id && rs.removed_node_left_child.id == -1) ||
            (rs.removed_node_parent.id == rs.removed_node_left_child.id) ||
            (rs.removed_node_left_child.id == id && rs.removed_node_parent.id == rs.removed_node_right_child.id)
            ) { // завершаем процесс удаления и балансировки
                printf("Ok\n");
                list_iter_remove(&li);
                return;
            }
            if (rs.replacing_with_node.id == id) // отправляем connect к левому сыну удаленного узла
                push_connect(rs.removed_node_left_child.id, rs.replacing_with_node.id,
                             rs.removed_node_left_child.main_address, rs.removed_node_left_child.ping_address);
            else if (rs.removed_node_left_child.id == id) // отправляем connect к правому сыну удаленного узла
                push_connect(rs.removed_node_right_child.id, rs.replacing_with_node.id,
                             rs.removed_node_left_child.main_address, rs.removed_node_left_child.ping_address);
        }
        else
            printf("Ok\n");
    }
}

void rec_disconnect(int id, int code, char* answer, int count_answers) {
    list_iter li = list_iter_begin(&remove_list);
    for (; list_iter_equal(li, list_iter_end(
            &remove_list)); list_iter_move_next(&li)) {
        remove_struct rs = list_iter_get(&li);
        if (rs.replacing_with_node.id == id)
            break;
    }
    if ((code == CODE_ERROR_NOT_FOUND && count_answers == 0) || code == CODE_ERROR_CUSTOM) {
        if (!list_iter_equal(li, list_iter_end(&remove_list))) { // если команда входит в цепочку удаления
            list_iter_remove(&li);
            printf("Balancing Error: Failed to disconnect the last node\n");
        } else
            printf("Error: Failed to disconnect\n");
    } else if (code == CODE_OK) {
        if (!list_iter_equal(li, list_iter_end(&remove_list))) { // если команда входит в цепочку удаления
            remove_struct rs = list_iter_get(&li);
            if (id == rs.replacing_with_node.id)
                push_remove(rs.removed_node.id);
            else if (id == rs.removed_node.id)
                push_connect(rs.replacing_with_node.id, rs.removed_node_parent.id, rs.replacing_with_node.main_address, rs.replacing_with_node.ping_address);
        } else
            printf("Ok\n");
    }
}

void push_create(int id, int parent_id, char* main_address, char* ping_address) {
    if (parent_id == C_NODE.id) {
        error_handler_zmq(mqn_connect(&CONTROL_NODE, id, main_address, ping_address));
        rec_create(id, CODE_OK, "OK", 0);
        return;
    }
    char uuid[37];
    gen_uuid(uuid);
    char str_parent_id[12];
    char str_rep_id[12];
    sprintf(str_parent_id, "%d", parent_id);
    sprintf(str_rep_id, "%d", id);
    char* push_message[7] = {uuid, "create", str_rep_id, str_parent_id, main_address, ping_address, NULL};
    error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    answer_count_push(uuid);
}

void push_connect(int id, int parent_id, char* main_address, char* ping_address) {
    if (parent_id == C_NODE.id) {
        error_handler_zmq(mqn_connect(&CONTROL_NODE, id, main_address, ping_address));
        rec_connect(id, CODE_OK, "OK", 0);
        return;
    }
    char uuid[37];
    gen_uuid(uuid);
    char str_parent_id[12];
    char str_rep_id[12];
    sprintf(str_parent_id, "%d", parent_id);
    sprintf(str_rep_id, "%d", id);
    char* push_message[7] = {uuid, "connect", str_rep_id, str_parent_id, main_address, ping_address, NULL};
    error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    answer_count_push(uuid);
}
void push_disconnect(int id) {
    if (CONTROL_NODE.left_child_id == id || CONTROL_NODE.right_child_id == id) {
        mqn_close(&CONTROL_NODE, id);
        rec_disconnect(id, CODE_OK, "OK", 0);
        return;
    }
    char uuid[37];
    gen_uuid(uuid);
    char str_id[12];
    sprintf(str_id, "%d", id);
    char* push_message[4] = {uuid, "disconnect", str_id, NULL};
    error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    answer_count_push(uuid);
}
void push_ping(int id) {
    if (C_NODE.id == id) {
        rec_ping(id, CODE_OK, "OK", 0);
        return;
    }
    char uuid[37];
    char str_id[12];
    gen_uuid(uuid);
    sprintf(str_id, "%d", id);
    char* push_message[4] = {uuid, "ping", str_id, NULL};
    error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    answer_count_push(uuid);
}
void push_remove(int id) {
    char uuid[37];
    char str_id[12];
    gen_uuid(uuid);
    sprintf(str_id, "%d", id);
    sprintf(str_id, "%d", id);
    char* push_message[4] = {uuid, "remove", str_id, NULL};
    error_handler_zmq(mqn_push_all(&CONTROL_NODE, (const char **) push_message));
    answer_count_push(uuid);
}

void receive_process(char* message[]) {
    char *uuid = message[0], *command = message[1], *id = message[2], *code = message[3], *answer = message[4];

    int *count_answers = hashmap_get(&answer_count_map, uuid);
    if (!count_answers)
        return;
    (*count_answers)--;

    int int_id = strtol(id, NULL, 10);
    int int_code = strtol(code, NULL, 10);

    if (!strcmp(command, "create"))
        rec_create(int_id, int_code, answer, *count_answers);
    else if (!strcmp(command, "remove"))
        rec_remove(int_id, int_code, answer, *count_answers);
    else if (!strcmp(command, "exec"))
        rec_exec(int_id, int_code, answer, *count_answers);
    else if (!strcmp(command, "ping"))
        rec_ping(int_id, int_code, answer, *count_answers);
    else if (!strcmp(command, "connect"))
        rec_connect(int_id, int_code, answer, *count_answers);
    else if (!strcmp(command, "disconnect"))
        rec_disconnect(int_id, int_code, answer, *count_answers);
    else
        printf("unknown command %s\n", command);
    if ((int_code == CODE_ERROR_NOT_FOUND && *count_answers == 0) || int_code == CODE_ERROR_CUSTOM || int_code == CODE_OK)
        hashmap_remove(&answer_count_map, uuid);
}

void *receive_worker(void* args) {
    char* message[5];
    while (true) {
        while (CONTROL_NODE.left_child_main_socket) {
            int size = mqn_receive(CONTROL_NODE.left_child_main_socket, message);
            if (size > 0)
                receive_process(message);
            else
                break;
        }
        while (CONTROL_NODE.right_child_main_socket) {
            int size = mqn_receive(CONTROL_NODE.right_child_main_socket, message);
            if (size > 0)
                receive_process(message);
            else
                break;
        }
    }
}

void *heartbeat_worker(void* args) {
    HASHMAP(char*, int) nodes_map;
    hashmap_init(&nodes_map, hashmap_hash_string, strcmp);
    clock_t start = clock();
    while (heartbeat_time > 0) {
        for (int i = 0; i < TREE.size; ++i) {
            char str_id[12];
            sprintf(str_id, "%d", TREE.buf[i].id);
            if (!hashmap_get(&nodes_map, str_id))
                hashmap_put(&nodes_map, str_id, 0);
        }
        char* map_key;
        int map_data;
        hashmap_foreach(map_key, map_data, &nodes_map) {
            hashmap_put(&nodes_map, map_key, map_data+1);
        }
        char* push_msg[2] = {"ping", NULL};
        if (CONTROL_NODE.left_child_ping_socket) mqn_push(CONTROL_NODE.left_child_ping_socket, (const char **) push_msg);
        if (CONTROL_NODE.right_child_ping_socket) mqn_push(CONTROL_NODE.right_child_ping_socket,
                                                           (const char **) push_msg);
        char* rec_msg[2]; // pong node_id
        while (clock() - start < heartbeat_time) {
            while (CONTROL_NODE.left_child_ping_socket) {
                int size = mqn_receive(CONTROL_NODE.left_child_ping_socket, rec_msg);
                if (size > 0 && !strcmp(rec_msg[0], "pong"))
                    hashmap_put(&nodes_map, rec_msg[1], 0);
                else
                    break;
            }
            while (CONTROL_NODE.right_child_ping_socket) {
                int size = mqn_receive(CONTROL_NODE.right_child_ping_socket, rec_msg);
                if (size > 0 && !strcmp(rec_msg[0], "pong"))
                    hashmap_put(&nodes_map, rec_msg[1], 0);
                else
                    break;
            }
            // nanosleep()
        }
        hashmap_foreach(map_key, map_data, &nodes_map) {
            if (map_data == 4)
                printf("Heartbeat: node %s is unavailable now\n", map_key);
        }
    }
}
