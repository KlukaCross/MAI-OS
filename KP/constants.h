#ifndef KP_CONSTANTS_H
#define KP_CONSTANTS_H

#define REQUEST_PIPE_NAME "req_pipe"
#define AUTHORIZATION_RESPONSE_PIPE_NAME "auth_res_pipe"
#define PIPE_PATH "/tmp/"
#define STATUS_OK "OK"
#define STATUS_ERROR "ERROR"

#define MILLISECONDS_SLEEP 300
#define SECONDS_SLEEP 0

#define NONE_USER_NAME "__NONE"
#define SYSTEM_NAME "SYSTEM"
#define COMMAND_ARGS_SEPARATOR '|'
#define COMMAND_SEPARATOR '\n'
#define MIN_NAME_SIZE 2
#define MAX_NAME_SIZE 20
#define MAX_MESSAGE_CHARACTERS 2048

#define CMD_HELP "help"
#define CMD_LOGIN "login"
#define CMD_LOGOUT "logout"
#define CMD_PRIVATE "private"
#define CMD_JOIN "join"
#define CMD_CREATE "create"
#define CMD_PUSH_MSG "push_msg"
#define CMD_GET_MSG "get_msg"
#define CMD_CHATS "chats"

#define ARG_GROUP "GROUP"
#define ARG_PRIVATE "PRIVATE"

#endif //KP_CONSTANTS_H
