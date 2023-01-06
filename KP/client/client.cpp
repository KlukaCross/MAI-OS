#include "../constants.h"
#include "client.h"
#include "../funcs/string_parser.h"
#include "../funcs/gen_uuid.h"
#define ERROR_USER_IS_NOT_LOGGED_IN "user is not logged in"

const struct timespec SLEEP_TIME = {SECONDS_SLEEP, MILLISECONDS_SLEEP*1000000L}; // sec nanosec

Client::Client(print_function print) {
    this->print_f = print;
    this->state = NotLogin;
    this->request_pipe = new FifoPipePut(std::string(PIPE_PATH)+REQUEST_PIPE_NAME);
    this->response_pipe = new FifoPipeGet(std::string(PIPE_PATH)+gen_uuid());
    this->last_update_id = '0';
}

void Client::start() {
    receive_loop();
}

void Client::receive_loop() {
    while (true) {
        std::string answer = this->response_pipe->get_message(COMMAND_SEPARATOR);
        if (answer.empty()) {
            nanosleep(&SLEEP_TIME, nullptr);
            continue;
        }
        answer_handler(answer);
    }
}

void Client::update_chat(const std::string& last_update_id) {
    std::string args = std::string((this->state == InGroupChat) ? CMD_ARG_GROUP : CMD_ARG_PRIVATE) + COMMAND_ARGS_SEPARATOR +
                       this->chat_name + COMMAND_ARGS_SEPARATOR + last_update_id;
    request(CMD_GET_MSG, args);
}

void Client::answer_handler(const std::string &answer) {
    // cmd status args...
    std::vector<std::string> string_p;
    string_parser(answer, 3, string_p, COMMAND_ARGS_SEPARATOR);
    std::string cmd = string_p[0];
    std::string status = string_p[1];
    std::string args = (string_p.size() == 2)? "" : string_p[2];
    if (status == STATUS_ERROR)
        print("Error: " + args, PRINT_SYSTEM_INFO_ERR);
    else if (cmd == CMD_LOGIN)
        rec_login(args);
    else if (cmd == CMD_PRIVATE)
        rec_private(args);
    else if (cmd == CMD_CREATE)
        rec_create_chat(args);
    else if (cmd == CMD_JOIN)
        rec_join(args);
    else if (cmd == CMD_GET_MSG)
        rec_get_msg(args);
    else if (cmd == CMD_CHATS)
        rec_chats(args);
}

void Client::cmd_login(const std::string &login) {
    if (this->state != NotLogin) {
        print("user is already logged in", PRINT_SYSTEM_INFO_ERR);
        return;
    }
    if (login.size() < MIN_NAME_SIZE || login.size() > MAX_NAME_SIZE) {
        print("Name size must be greater than " + std::to_string(MIN_NAME_SIZE - 1) + " and less than " +
                std::to_string(MAX_NAME_SIZE+1), PRINT_SYSTEM_INFO_ERR);
        return;
    }
    if (login.find(COMMAND_ARGS_SEPARATOR) != -1) {
        print("Name contains forbidden characters", PRINT_SYSTEM_INFO_ERR);
        return;
    }
    this->login = login;
    request(CMD_LOGIN, login);
}

void Client::cmd_logout() {
    if (this->state == NotLogin) {
        print(ERROR_USER_IS_NOT_LOGGED_IN, PRINT_SYSTEM_INFO_ERR);
        return;
    }
    request(CMD_LOGOUT, "");
    this->state = NotLogin;
    print("logout successful", PRINT_SYSTEM_INFO_OK);
}

void Client::cmd_join(const std::string &chat_name) {
    if (this->state == NotLogin) {
        print(ERROR_USER_IS_NOT_LOGGED_IN, PRINT_SYSTEM_INFO_ERR);
        return;
    }
    request(CMD_JOIN, chat_name);
}

void Client::cmd_private(const std::string &login) {
    if (this->state == NotLogin) {
        print(ERROR_USER_IS_NOT_LOGGED_IN, PRINT_SYSTEM_INFO_ERR);
        return;
    }
    request(CMD_PRIVATE, login);
}

void Client::cmd_create_chat(const std::string &chat_name) {
    if (this->state == NotLogin) {
        print(ERROR_USER_IS_NOT_LOGGED_IN, PRINT_SYSTEM_INFO_ERR);
        return;
    }
    if (chat_name.size() < MIN_NAME_SIZE || chat_name.size() > MAX_NAME_SIZE) {
        print("Chat name size must be greater than " + std::to_string(MIN_NAME_SIZE - 1) + " and less than " +
                std::to_string(MAX_NAME_SIZE+1), PRINT_SYSTEM_INFO_ERR);
        return;
    }
    if (chat_name.find(COMMAND_ARGS_SEPARATOR) != -1) {
        print("Chat name contains forbidden characters", PRINT_SYSTEM_INFO_ERR);
        return;
    }
    request(CMD_CREATE, chat_name);
}

void Client::cmd_push(const std::string &text) {
    if (this->state == NotLogin) {
        print(ERROR_USER_IS_NOT_LOGGED_IN, PRINT_SYSTEM_INFO_ERR);
        return;
    }
    std::string req = text;
    if (text.find(COMMAND_ARGS_SEPARATOR) != -1) {
        print("Text contains forbidden characters", PRINT_SYSTEM_INFO_ERR); // лучше использовать экранирование
        return;
    }
    if (text.size() > MAX_MESSAGE_CHARACTERS) {
        print("Message size must be less than " + std::to_string(MAX_MESSAGE_CHARACTERS + 1), PRINT_SYSTEM_INFO_ERR);
        print("The message is truncated", PRINT_SYSTEM_INFO_ERR);
        req = req.substr(0, MAX_MESSAGE_CHARACTERS);
    }
    std::string args = std::string((this->state == InGroupChat) ? CMD_ARG_GROUP : CMD_ARG_PRIVATE) + COMMAND_ARGS_SEPARATOR + chat_name + COMMAND_ARGS_SEPARATOR + req;
    request(CMD_PUSH_MSG, args);
}

void Client::cmd_chats() {
    request(CMD_CHATS, "");
}

void Client::request(const std::string &command, const std::string &arguments) {
    std::string name = (this->state == NotLogin)? NONE_USER_NAME : this->login;
    std::string args;
    if (!arguments.empty()) args += COMMAND_ARGS_SEPARATOR;
    args += arguments;
    this->request_pipe->put_message(command + COMMAND_ARGS_SEPARATOR + this->response_pipe->pipe_name + COMMAND_ARGS_SEPARATOR + name + args, COMMAND_SEPARATOR);
}

void Client::rec_login(const std::string &args) {
    this->state = InMainMenu;
    print("successful authorization", PRINT_SYSTEM_INFO_OK);
}

void Client::rec_join(const std::string &args) {
    this->state = InGroupChat;
    this->chat_name = args;
    this->last_update_id = '0';
    print("Group chat " + args, PRINT_SYSTEM_INFO_OK);
}

void Client::rec_private(const std::string &args) {
    this->state = InPrivateChat;
    this->chat_name = args;
    this->last_update_id = '0';
    print("Chat with the user " + args, PRINT_SYSTEM_INFO_OK);
}

void Client::rec_create_chat(const std::string &args) {
    this->state = InGroupChat;
    this->chat_name = args;
    this->last_update_id = '0';
    print("Group chat " + args, PRINT_SYSTEM_INFO_OK);
}

void Client::rec_chats(const std::string &args) {
    // is_public1|chat_name1|user_name1|last_message1|is_public2|chat_name2|user_name2|last_message2|...
    if (args.empty())
        return;
    print(args, PRINT_CHAT_INFO);
}

void Client::rec_get_msg(const std::string &args) {
    // user_name1|message1|user_name2|message2|...
    std::vector<std::string> args_p;
    string_parser(args, 2, args_p, COMMAND_ARGS_SEPARATOR);
    this->last_update_id = args_p[0];
    if (args_p[1].empty())
        return;
    print(args_p[1], PRINT_MESSAGE);
}

void Client::print(const std::string& text, int print_type, const std::string& end) {
    this->print_f(text, print_type, end);
}

void Client::print(const char c, int print_type, const std::string &end) {
    std::string a;
    a += c;
    print(a, print_type, end);
}

