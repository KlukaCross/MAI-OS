#include <iostream>
#include <pthread.h>
#include "constants.h"
#include "client.h"
#include "funcs/string_parser.h"
#include "funcs/gen_uuid.h"
#define ERROR_USER_IS_NOT_LOGGED_IN "user is not logged in"

void* console_reader(void* args);
void* client_receiver(void* args);
void* client_updater(void* args);

const struct timespec SLEEP_TIME = {SECONDS_SLEEP, MILLISECONDS_SLEEP*1000000L}; // sec nanosec

int main() {
    auto *client = new Client();
    pthread_t console_reader_p;
    pthread_t client_receiver_p;
    pthread_t client_updater_p;
    pthread_create(&console_reader_p, nullptr, console_reader, client);
    pthread_create(&client_receiver_p, nullptr, client_receiver, client);
    pthread_create(&client_updater_p, nullptr, client_updater, client);
    pthread_join(console_reader_p, nullptr);
    pthread_join(client_receiver_p, nullptr);
    pthread_join(client_updater_p, nullptr);
    delete client;
    return 0;
}

void print(const std::string& text) {
    std::cout << text << std::endl;
}

void print_help(ClientStates state) {
    print(std::string(CMD_HELP) + " - вывести эту справку");
    if (state == NotLogin) {
        print(std::string(CMD_LOGIN) + " LOGIN - авторизоваться под логином LOGIN\n");
    } else if (state == InMainMenu) {
        print(std::string(CMD_CHATS) + " - посмотреть список чатов, в которых было общение");
        print(std::string(CMD_PRIVATE) + " LOGIN - войти в личный чат с пользователем LOGIN");
        print(std::string(CMD_JOIN) + " CHAT_NAME - войти в групповой чат с именем CHAT_NAME");
        print(std::string(CMD_CREATE) + " CHAT_NAME - создать групповой чат с именем CHAT_NAME");
        print(std::string(CMD_LOGOUT) + " - выйти из системы");
    }
}

void* console_reader(void* args) {
    auto *client = static_cast<Client *>(args);
    print_help(client->state);
    std::vector<std::string> args_p;
    while (true) {
        nanosleep(&SLEEP_TIME, nullptr);
        std::string text;
        std::getline(std::cin, text);
        if (client->state == InPrivateChat || client->state == InGroupChat) {
            if (!text.empty())
                client->cmd_push(text);
            continue;
        }
        string_parser(text, 2, args_p, ' ');
        std::string cmd = args_p[0];
        if (cmd == CMD_HELP)
            print_help(client->state);
        else if (cmd == CMD_LOGIN) {
            client->cmd_login(args_p[1]);
        } else if (cmd == CMD_LOGOUT)
            client->cmd_logout();
        else if (cmd == CMD_PRIVATE) {
            client->cmd_private(args_p[1]);
        } else if (cmd == CMD_JOIN) {
            client->cmd_join(args_p[1]);
        } else if (cmd == CMD_CREATE) {
            client->cmd_create_chat(args_p[1]);
        } else if (cmd == CMD_CHATS) {
            client->cmd_chats();
        }
        args_p.clear();
    }
    return nullptr;
}
void* client_receiver(void* args) {
    auto *client = static_cast<Client *>(args);
    client->start();
    return nullptr;
}

void* client_updater(void* args) {
    auto *client = static_cast<Client *>(args);
    while (true) {
        if (client->state == InPrivateChat || client->state == InGroupChat)
            client->update_chat(client->last_update_id);
        nanosleep(&SLEEP_TIME, nullptr);
    }
    return nullptr;
}

Client::Client() {
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
        std::string answer;
        answer = this->response_pipe->get_message();
        if (answer.empty()) {
            nanosleep(&SLEEP_TIME, nullptr);
            continue;
        }
        answer_handler(answer);
    }
}

void Client::update_chat(const std::string& last_update_id) {
    std::string args = std::string((this->state == InGroupChat) ? ARG_GROUP : ARG_PRIVATE) + COMMAND_SEPARATOR + this->chat_name + COMMAND_SEPARATOR + last_update_id;
    request(CMD_GET_MSG, args);
}

void Client::answer_handler(const std::string &answer) {
    // cmd status args...
    std::vector<std::string> string_p;
    string_parser(answer, 3, string_p, COMMAND_SEPARATOR);
    std::string cmd = string_p[0];
    std::string status = string_p[1];
    std::string args = (string_p.size() == 2)? "" : string_p[2];
    if (status == STATUS_ERROR)
        print("Error: " + args);
    else if (cmd == CMD_LOGIN) {
        this->state = InMainMenu;
        print("successful authorization");
        print_help(this->state);
    } else if (cmd == CMD_PRIVATE) {
        this->state = InPrivateChat;
        this->chat_name = args;
        this->last_update_id = '0';
        print("Chat with the user " + args);
    } else if (cmd == CMD_CREATE || cmd == CMD_JOIN) {
        this->state = InGroupChat;
        this->chat_name = args;
        this->last_update_id = '0';
        print("Group chat " + args);
    } else if (cmd == CMD_GET_MSG) {
        std::vector<std::string> args_p;
        string_parser(args, 2, args_p, COMMAND_SEPARATOR);
        this->last_update_id = args_p[0];
        if (args_p[1].empty())
            return;
        std::string res;
        for (char & i : args_p[1]) {
            if (i == COMMAND_SEPARATOR)
                i = '\n';
        }
        print(args_p[1]);
    } else if (cmd == CMD_CHATS) {
        if (args.empty())
            return;
        std::string res;
        for (char & i : args) {
            if (i == COMMAND_SEPARATOR)
                i = '\n';
        }
        print(args);
    }
}

void Client::cmd_login(const std::string &login) {
    if (this->state != NotLogin) {
        std::cout << "user is already logged in\n";
        return;
    }
    this->login = login;
    request(CMD_LOGIN, login);
}

void Client::cmd_logout() {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN << std::endl;
        return;
    }
    request(CMD_LOGOUT, "");
    this->state = NotLogin;
    print("logout successful");
}

void Client::cmd_join(const std::string &chat_name) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN << std::endl;
        return;
    }
    request(CMD_JOIN, chat_name);
}

void Client::cmd_private(const std::string &login) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN << std::endl;
        return;
    }
    request(CMD_PRIVATE, login);
}

void Client::cmd_create_chat(const std::string &chat_name) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN << std::endl;
        return;
    }
    request(CMD_CREATE, chat_name);
}

void Client::cmd_push(const std::string &text) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN << std::endl;
        return;
    }
    std::string args = std::string((this->state == InGroupChat) ? ARG_GROUP : ARG_PRIVATE) + COMMAND_SEPARATOR + chat_name + COMMAND_SEPARATOR + text;
    request(CMD_PUSH_MSG, args);
}

void Client::cmd_chats() {
    request(CMD_CHATS, "");
}

void Client::request(const std::string &command, const std::string &arguments) {
    std::string name = (this->state == NotLogin)? NONE_USER_NAME : this->login;
    std::string args;
    if (!arguments.empty()) args += COMMAND_SEPARATOR;
    args += arguments;
    this->request_pipe->put_message(command + COMMAND_SEPARATOR + this->response_pipe->pipe_name + COMMAND_SEPARATOR + name + args);
}
