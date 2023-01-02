#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <uuid/uuid.h>
#include <time.h>
#include <assert.h>
#include <wait.h>
#include "structures/balanced_tree.h"
#include "structures/mq_node.h"
#include "errors/error_handler.h"
#include "hashmap/include//hashmap.h"
#include "structures/rm_list.h"

#define MILLISECONDS_SLEEP 50
#define SECONDS_SLEEP 0

//#define DEBUG

void print_help() {
    printf("USAGE:\n"
           "\tcreate ID - create node with id=ID\n"
           "\tremove ID - remove node with id=ID\n"
           "\texec ID NAME [VALUE] - get value or set VALUE by the key NAME\n"
           "\theartbeat TIME - each node begins to report once every TIME milliseconds that it is operational. If "
           "there is no signal from the node for 4*TIME milliseconds, then a line about the node's unavailability is displayed. "
           "If TIME=0 then heartbeat is disabled\n");
}

void gen_address(char* rep_address, char* req_address);
char* gen_uuid();
char* int_to_str(int a);
char* str_copy(char *s);
void answer_count_push(char *uuid);

void cmd_create(int id); // create id
void cmd_remove(int id); // remove id
void cmd_exec(bool is_set, int id, char* name, int value); // exec id name value
bool cmd_heartbeat(int time); // heartbeat time

void rec_create(int id, int code, char* answer, int count_answers);
void rec_exec(int id, int code, char* answer, int count_answers);
void rec_remove(char* command, int id, int code, char* answer, int count_answers);

void push_create(int id, int parent_id, char* main_address, char* ping_address);
void push_next_remove(remove_struct rms);

void *receive_worker(void* args);
void *heartbeat_worker(void* args);

int address_shift;
b_tree TREE;
mq_node CONTROL_NODE;
b_tree_node C_NODE;

int heartbeat_time = 0;

HASHMAP(char, int) answer_count_map;
HASHMAP(char, int) nodes_ping_map;
rm_list remove_list;

const struct timespec SLEEP_TIME = {SECONDS_SLEEP, MILLISECONDS_SLEEP*1000000L}; // sec nanosec

int main (void)
{
    btree_init(&TREE);
    C_NODE.id = -1;
    C_NODE.pid = getpid();
    btree_add(&TREE, C_NODE);
    hashmap_init(&answer_count_map, hashmap_hash_string, strcmp);
    hashmap_init(&nodes_ping_map, hashmap_hash_string, strcmp);
    list_init(&remove_list);

    address_shift = 5000;
    char *main_address = malloc(sizeof(char)*32);
    gen_address(main_address, NULL);
    char *ping_address = malloc(sizeof(char)*32);
    gen_address(ping_address, NULL);
    mqn_init(&CONTROL_NODE);
    mqn_bind(&CONTROL_NODE, main_address, ping_address);

    pthread_t receive_thread;
    pthread_t heartbeat_thread;

    error_handler(!pthread_create(&receive_thread, NULL, receive_worker, NULL), "create thread error");
    error_handler(!pthread_detach(receive_thread), "detach thread error");

    int arg1, arg3;
    char *arg2 = NULL;
    char cmd[10] = "";
    print_help();
    while (true) {
        if (strlen(cmd) == 0)
            if (scanf("%s", cmd) == EOF) break;
        if (!strcmp(cmd, "create")) {
            if (scanf("%d", &arg1) == EOF) break;
            cmd_create(arg1);
        } else if (!strcmp(cmd, "remove")) {
            if (scanf("%d", &arg1) == EOF) break;
            cmd_remove(arg1);
        } else if (!strcmp(cmd, "exec")) {
            if (scanf("%d %ms", &arg1, &arg2) == EOF) break;
            int c = getchar();
            if (c == ' ') {
                if (scanf("%d", &arg3) == EOF) break;
                cmd_exec(true, arg1, arg2, arg3);
            } else
                cmd_exec(false, arg1, arg2, arg3);
        } else if (!strcmp(cmd, "heartbeat")) {
            if (scanf("%d", &arg1) == EOF) break;
            bool res_cmd = cmd_heartbeat(arg1);
            if (res_cmd) {
                error_handler(!pthread_create(&heartbeat_thread, NULL, heartbeat_worker, NULL), "create thread error");
                error_handler(!pthread_detach(heartbeat_thread), "detach thread error");
            }
        }
        else {
            printf("unknown command - %s\n", cmd);
            int c;
            do {
                c = getchar();
                if (c == EOF) break;
            } while (c != '\n');
            print_help();
        }
        cmd[0] = '\0';
    }
    free(arg2);
    for (int i = 1; i < TREE.size; ++i) {
        kill(TREE.buf[i].pid, SIGKILL);
        waitpid(TREE.buf[i].pid, NULL, 0);
    }
    return 0;
}

void gen_address(char *rep_address, char *req_address) {
    char *buff = int_to_str(address_shift++);
    rep_address[0] = '\0';
    strcat(rep_address, "tcp://*:");
    strcat(rep_address, buff);
    if (req_address != NULL) {
        req_address[0] = '\0';
        strcat(req_address, "tcp://localhost:");
        strcat(req_address, buff);
    }
    free(buff);
}

char* gen_uuid() {
    char *uuid = malloc(sizeof(char)*37);
    assert(uuid != NULL);
    uuid_t uu;
    uuid_generate_random(uu);
    uuid_unparse(uu, uuid);
    return uuid;
}
char* int_to_str(int a) {
    char* res = malloc(sizeof(char)*12);
    assert(res != NULL);
    sprintf(res, "%d", a);
    return res;
}
char* str_copy(char *s) {
    if (s == NULL)
        return NULL;
    int len_s = strlen(s);
    char* res = malloc((len_s+1)*sizeof(char));
    assert(res != NULL);
    memcpy(res, s, len_s*sizeof(char));
    res[len_s] = '\0';
    return res;
}

void answer_count_push(char *uuid) {
    int *val = malloc(sizeof(int));
    char *key = malloc(sizeof(char)*(strlen(uuid)+1));
    memcpy(key, uuid, strlen(uuid));
    key[strlen(uuid)] = '\0';
    *val = (TREE.size+1)/2;
    assert(hashmap_put(&answer_count_map, key, val) == 0);
}

void cmd_create(int id) {
    if (btree_find(&TREE, id).pid != -1) { // если такой узел уже есть
        printf("Error: Already exists\n");
        return;
    }

    char *rep_main_address = malloc(sizeof(char)*32);
    char *req_main_address = malloc(sizeof(char)*32);
    gen_address(rep_main_address, req_main_address);
    char *rep_ping_address = malloc(sizeof(char)*32);
    char *req_ping_address = malloc(sizeof(char)*32);
    gen_address(rep_ping_address, req_ping_address);

    int pid = fork();
    error_handler(pid >= 0, "node create error\n");
    if (pid == 0) {
        char *str_id = int_to_str(id);
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

    remove_struct rms;
    rms.remove_node_id = removed_node.id;
    if (last_node.id == removed_node.id) {
        // если удаляемый узел - это последний, то используем сразу команду remove, затем disconnect его от родителя
        char *str_id = int_to_str(removed_node.id);
        char* cmd[6] = {str_copy("remove"), str_id, NULL,
                        str_copy("disconnect"), str_copy(str_id), NULL};
        rms._len_commands = 6;
        int len_cmd = sizeof(char*)*rms._len_commands;

        rms.commands = malloc(len_cmd);
        rms.number_commands = 2;
        rms.now_command = 0;
        memcpy(rms.commands, cmd, len_cmd);
    }
    else {
        // иначе: проверяем, доступен ли последний узел, затем отсоединяем его, удаляем удаляемый узел,
        // отcоединяем его родителя от него, подключаем родителя удаляемого узла к последнему, последний подключаем к сыновьям удаленного узла
        char *remove_str_id = int_to_str(removed_node.id);
        char *last_str_id = int_to_str(last_node.id);
        char *remove_parent_str_id = int_to_str(parent_of_removed_node.id);
        char *remove_left_child_id = int_to_str(left_child_of_removed_node.id);
        char *remove_right_child_id = int_to_str(right_child_of_removed_node.id);
        char* cmd[31] = {str_copy("ping"), remove_str_id, NULL,
                         str_copy("disconnect"), last_str_id, NULL,
                         str_copy("remove"), str_copy(remove_str_id), NULL,
                         str_copy("disconnect"), str_copy(remove_str_id), NULL,
                         str_copy("connect"), str_copy(last_str_id), remove_parent_str_id, str_copy(last_node.main_address),
                         str_copy(last_node.ping_address), NULL,
                         str_copy("connect"), remove_left_child_id, str_copy(last_str_id), str_copy(left_child_of_removed_node.main_address),
                         str_copy(left_child_of_removed_node.ping_address), NULL,
                         str_copy("connect"), remove_right_child_id, str_copy(last_str_id), str_copy(right_child_of_removed_node.main_address),
                         str_copy(right_child_of_removed_node.ping_address), NULL};
        rms._len_commands = 31;
        rms.number_commands = 5;
        if (left_child_of_removed_node.id != last_node.id && left_child_of_removed_node.id != -1)
            rms.number_commands++;
        if (right_child_of_removed_node.id != last_node.id && right_child_of_removed_node.id != -1)
            rms.number_commands++;
        int len_cmd = sizeof(char*)*rms._len_commands;

        rms.commands = malloc(len_cmd);
        rms.now_command = 0;
        memcpy(rms.commands, cmd, len_cmd);
    }
    list_iter li = list_iter_begin(&remove_list);
    list_iter_insert_before(&li, rms);
    push_next_remove(rms);
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

    char *str_id = int_to_str(id);
    char *uuid = gen_uuid();
    answer_count_push(uuid);
    if (is_set) {
        char *str_value = int_to_str(value);
        char* push_message[6] = {uuid, "exec", str_id, name, str_value, NULL};
        mqn_push_all(&CONTROL_NODE, push_message);
        free(str_value);
    } else {
        char* push_message[5] = {uuid, "exec", str_id, name, NULL};
        mqn_push_all(&CONTROL_NODE, push_message);
    }
    free(uuid);
    free(str_id);
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
        return false;
    }
    heartbeat_time = time;
    return true;
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
    } else if (code == CODE_OK) {
        int *v = malloc(sizeof(int));
        char* str_id = int_to_str(id);
        *v = 0;
        hashmap_put(&nodes_ping_map, str_id, v);
        printf("Ok: %d\n", created_node.pid);
    }
}

void rec_remove(char* command, int id, int code, char* answer, int count_answers) {
    if (code == CODE_ERROR_NOT_FOUND && count_answers > 0)
        return;
    char *str_id = int_to_str(id);
    list_iter li = list_iter_begin(&remove_list);
    remove_struct rs;
    for (; !list_iter_equal(li, list_iter_end(&remove_list)); list_iter_move_next(&li)) {
        rs = list_iter_get(&li);
        int cmd_it = 0;
        int str_it = 0;
        while (cmd_it < rs.now_command) {
            if (rs.commands[str_it] == NULL)
                cmd_it++;
            str_it++;
        }
        if (!strcmp(command, rs.commands[str_it]) && !strcmp(str_id, rs.commands[str_it+1]))
            break;
    }

    if (list_iter_equal(li, list_iter_end(&remove_list)))
        printf("Error: Not found in remove_list\n");
    rs.now_command++;
    list_iter_set(&li, rs);
    if ((code == CODE_ERROR_NOT_FOUND && count_answers == 0) || code == CODE_ERROR_CUSTOM) {
        list_iter_remove(&li);
        for (int i = 0; i < rs._len_commands; ++i) {
            free(rs.commands[i]);
        }
        free(rs.commands);
        printf("Error: Failed to %s %s: %s\n", command, str_id, answer);
    }
    else if (code == CODE_OK) {
        if (rs.now_command >= rs.number_commands) {
            list_iter_remove(&li);
            for (int i = 0; i < rs._len_commands; ++i) {
                free(rs.commands[i]);
            }
            free(rs.commands);
            char* remove_node_str = int_to_str(rs.remove_node_id);
            waitpid(btree_find(&TREE, rs.remove_node_id).pid, NULL, 0);
            if (hashmap_get(&nodes_ping_map, remove_node_str) != NULL)
                hashmap_remove(&nodes_ping_map, remove_node_str);
            btree_remove(&TREE, rs.remove_node_id);
            printf("Ok\n");
            free(remove_node_str);
        } else
            push_next_remove(rs);
    }
    free(str_id);
}
void rec_exec(int id, int code, char* answer, int count_answers) {
    if (code == CODE_ERROR_NOT_FOUND && count_answers == 0 || code == CODE_ERROR_CUSTOM)
        printf("Error:%d: %s\n", id, answer);
    else if (code == CODE_OK)
        printf("Ok:%d: %s\n", id, answer);
}

void push_create(int id, int parent_id, char* main_address, char* ping_address) {
    if (parent_id == C_NODE.id) {
        mqn_connect(&CONTROL_NODE, id, main_address, ping_address);
#ifdef DEBUG
        printf("control_node reply OK for command create\n");
        fflush(stdout);
#endif
        rec_create(id, CODE_OK, "OK", 0);
        return;
    }
    char *uuid = gen_uuid();
    char *str_parent_id = int_to_str(parent_id);
    char *str_rep_id = int_to_str(id);
    char* push_message[7] = {uuid, "create", str_rep_id, str_parent_id, main_address, ping_address, NULL};
    answer_count_push(uuid);
    mqn_push_all(&CONTROL_NODE, push_message);
    free(uuid);
    free(str_parent_id);
    free(str_rep_id);
}

void push_next_remove(remove_struct rms) { // отправляет следующую команду в цепочке удаления
    int cmd_it=0;
    int str_it=0;
    while (cmd_it < rms.now_command) {
        if (rms.commands[str_it] == NULL)
            cmd_it++;
        str_it++;
    }
    char *cmd = rms.commands[str_it];
    char *uuid = gen_uuid();
    char* push_message[7] = {uuid, cmd, NULL, NULL, NULL, NULL, NULL};
    if (!strcmp(cmd, "remove"))
        push_message[2] = rms.commands[str_it+1];
    else if (!strcmp(cmd, "disconnect")) {
        int node_id = strtol(rms.commands[str_it+1], NULL, 10);
        if (CONTROL_NODE.left_child_id == node_id || CONTROL_NODE.right_child_id == node_id) {
            mqn_close(&CONTROL_NODE, node_id);
            rec_remove(cmd, node_id, CODE_OK, "OK", 0);
#ifdef DEBUG
            printf("control_node reply OK for command disconnect\n");
            fflush(stdout);
#endif
            return;
        }
        push_message[2] = rms.commands[str_it+1];
    } else if (!strcmp(cmd, "connect")) {
        char* parent_id = rms.commands[str_it+2];
        int node_id = strtol(rms.commands[str_it+1], NULL, 10);
        if (strtol(parent_id, NULL, 10) == C_NODE.id) {
            mqn_connect(&CONTROL_NODE, node_id, rms.commands[str_it+3], rms.commands[str_it+4]);
            rec_remove(cmd, node_id, CODE_OK, "OK", 0);
#ifdef DEBUG
            printf("control_node reply OK for command connect\n");
            fflush(stdout);
#endif
            return;
        }
        push_message[2] = rms.commands[str_it+1];
        push_message[3] = rms.commands[str_it+2];
        push_message[4] = rms.commands[str_it+3];
        push_message[5] = rms.commands[str_it+4];
    } else if (!strcmp(cmd, "ping")) {
        char* ping_id = rms.commands[str_it+1];
        if (C_NODE.id == strtol(ping_id, NULL, 10)) {
            rec_remove(cmd, C_NODE.id, CODE_OK, "OK", 0);
#ifdef DEBUG
            printf("control_node reply OK for command ping\n");
            fflush(stdout);
#endif
            return;
        }
        push_message[2] = ping_id;
    }
    answer_count_push(uuid);
    mqn_push_all(&CONTROL_NODE, push_message);
    free(uuid);
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
    else if (!strcmp(command, "remove") || !strcmp(command, "ping") || !strcmp(command, "connect") || !strcmp(command, "disconnect"))
        rec_remove(command, int_id, int_code, answer, *count_answers);
    else if (!strcmp(command, "exec"))
        rec_exec(int_id, int_code, answer, *count_answers);
    else
        printf("unknown command %s\n", command);
    if ((int_code == CODE_ERROR_NOT_FOUND && *count_answers == 0) || int_code == CODE_ERROR_CUSTOM || int_code == CODE_OK)
        hashmap_remove(&answer_count_map, uuid);
}

void receive_from_socket(void *socket) {
    char *message[5] = {NULL, NULL, NULL, NULL, NULL};
    while (socket) {
        int size = mqn_receive(socket, message);
        if (size > 0)
            receive_process(message);
        else
            break;
    }
}
void *receive_worker(void* args) {
    while (true) {
        receive_from_socket(CONTROL_NODE.left_child_main_socket);
        receive_from_socket(CONTROL_NODE.right_child_main_socket);
        nanosleep(&SLEEP_TIME, NULL);
    }
}

void receive_pong_from_socket(void *socket) {
    char* rec_msg[2] = {NULL, NULL}; // pong node_id
    while (socket) {
        int size = mqn_receive(socket, rec_msg);
        if (size > 0 && !strcmp(rec_msg[0], "pong")) {
            int *val = hashmap_get(&nodes_ping_map, rec_msg[1]);
            if (val)
                *val = 0;
        }
        else
            break;
    }
}
void *heartbeat_worker(void* args) {
    clock_t start;
    while (heartbeat_time > 0) {
        char* push_msg[2] = {"ping", NULL};
        int *v;
        hashmap_foreach_data(v, &nodes_ping_map) {
            ++(*v);
        }
        if (CONTROL_NODE.left_child_ping_socket) mqn_push(CONTROL_NODE.left_child_ping_socket, push_msg);
        if (CONTROL_NODE.right_child_ping_socket) mqn_push(CONTROL_NODE.right_child_ping_socket,push_msg);
        start = clock();
        while (clock() - start < heartbeat_time) {
            receive_pong_from_socket(CONTROL_NODE.left_child_ping_socket);
            receive_pong_from_socket(CONTROL_NODE.right_child_ping_socket);
            nanosleep(&SLEEP_TIME, NULL);
        }
        const char *map_key;
        int *map_data;
        hashmap_foreach(map_key, map_data, &nodes_ping_map) {
            if (*map_data == 4)
                printf("Heartbeat: node %s is unavailable now\n", map_key);
        }
    }
}
