#include <iostream>
#include <pthread.h>
#include "constants.h"
#include "client.h"
#define ERROR_USER_IS_NOT_LOGGED_IN "user is not logged in\n"

void* console_reader(void* args);
void* client_worker(void* args);

int main() {
    auto *client = new Client();
    pthread_t console_reader_p;
    pthread_t client_worker_p;
    pthread_create(&console_reader_p, nullptr, console_reader, client);
    pthread_create(&client_worker_p, nullptr, client_worker, client);
    pthread_join(console_reader_p, nullptr);
    pthread_join(client_worker_p, nullptr);
    delete client;
    return 0;
}

void print_help(ClientStates state) {
    std::cout << "help - вывести эту справку\n";
    if (state == NotLogin) {
        std::cout << "login LOGIN - авторизоваться под логином LOGIN\n";
    } else if (state == InMainMenu) {
        std::cout << "private LOGIN - войти в приватный чат с пользователем LOGIN\n";
        std::cout << "my - посмотреть список личных сообщений\n";
        std::cout << "join CHAT_NAME - войти в групповой чат с именем CHAT_NAME\n";
        std::cout << "create CHAT_NAME - создать групповой чат с именем CHAT_NAME\n";
        std::cout << "logout - выйти из системы\n";
    }
}

void* console_reader(void* args) {
    auto *client = static_cast<Client *>(args);
    print_help(client->state);
    while (true) {
        if (client->state == InPrivateChat || client->state == InGroupChat) {
            std::string text;
            std::getline(std::cin, text);
            client->cmd_push(text);
        } else {
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "help")
                print_help(client->state);
            else if (cmd == "login") {
                std::string login;
                std::cin >> login;
                client->cmd_login(login);
            } else if (cmd == "logout")
                client->cmd_logout();
            else if (cmd == "private") {
                std::string user;
                std::cin >> user;
                client->cmd_private(user);
            } else if (cmd == "join") {
                std::string chat_name;
                std::cin >> chat_name;
                client->cmd_join(chat_name);
            } else if (cmd == "create") {
                std::string chat_name;
                std::cin >> chat_name;
                client->cmd_create_chat(chat_name);
            }
        }
    }
    return nullptr;
}
void* client_worker(void* args) {
    auto *client = static_cast<Client *>(args);
    client->start();
    return nullptr;
}

Client::Client() {
    this->state = NotLogin;
    this->authorization_pipe = new FifoPipe(CONNECTION_PIPE_NAME_IN, CONNECTION_PIPE_NAME_OUT);
    this->main_pipe = nullptr;
    this->last_update_timestamp = 0;
}

void Client::start() {
    loop();
}

void Client::loop() {
    std::string answer;
    while (true) {
        if (this->state == NotLogin) {
            answer = this->authorization_pipe->get_message();
        }
        else if (this->state == InMainMenu) {
            answer = this->main_pipe->get_message();
        } else if (this->state == InPrivateChat){
            update_private_chat(last_update_timestamp);
            this->last_update_timestamp = std::time(nullptr);
            answer = this->main_pipe->get_message();
        }
        else if (this->state == InGroupChat){
            update_group_chat(last_update_timestamp);
            this->last_update_timestamp = std::time(nullptr);
            answer = this->main_pipe->get_message();
        }
        answer_handler(answer);
        // todo: проверка, есть ли сообщение в пайпе
        // todo: задержка
    }
}

void Client::update_group_chat(time_t timestamp) {
    std::string cmd = std::string("get_group_messages ") + this->chat_name + " " + std::to_string(timestamp);
    this->main_pipe->put_message(cmd);
}

void Client::update_private_chat(time_t timestamp) {
    std::string cmd = std::string("get_private_messages ") + this->login + " " + std::to_string(timestamp);
    this->main_pipe->put_message(cmd);
}

void Client::answer_handler(const std::string &answer) {
    // cmd status args...
    int ind_to_cmd = answer.find(' ');
    int ind_to_status = answer.find(' ', ind_to_cmd+1);
    std::string cmd = answer.substr(0, ind_to_cmd);
    std::string status = answer.substr(ind_to_cmd+1, ind_to_status);
    if (status == STATUS_ERROR) {
        std::cout << "Error: " << answer.substr(ind_to_status+1, answer.size()) << std::endl;
        return;
    }
    if (cmd == "login") {
        int ind_to_pipe_get = answer.find(' ', ind_to_status+1);
        int ind_to_pipe_put = answer.find(' ', ind_to_pipe_get+1);
        this->set_main_pipe(answer.substr(ind_to_status+1, ind_to_pipe_get-ind_to_status),
                            answer.substr(ind_to_pipe_get+1, ind_to_pipe_put-ind_to_pipe_get));
        this->state = InMainMenu;
        std::cout << "successful authorization\n";
        print_help(this->state);
    } else if (cmd == "private") {
        std::string user_name = answer.substr(ind_to_status + 1, answer.size() - ind_to_status);
        this->state = InPrivateChat;
        this->chat_name = user_name;
        this->last_update_timestamp = 0;
        std::cout << "Chat with the user " << user_name << std::endl;
    } else if (cmd == "create" || cmd == "join") {
        std::string chat_name = answer.substr(ind_to_status+1, answer.size()-ind_to_status);
        this->state = InGroupChat;
        this->chat_name = chat_name;
        this->last_update_timestamp = 0;
        std::cout << "Group chat " << chat_name << std::endl;
    } else if (cmd == "get_group_messages" || cmd == "get_private_messages") {
        std::cout << answer.substr(ind_to_status+1, answer.size() - ind_to_status);
    }
}

void Client::cmd_login(const std::string &login) {
    if (this->state != NotLogin) {
        std::cout << "user is already logged in\n";
        return;
    }
    std::string cmd = std::string("login ") + login;
    this->main_pipe->put_message(cmd);
}

void Client::cmd_logout() {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN;
        return;
    }
    std::string cmd = "logout";
    this->main_pipe->put_message(cmd);
}

void Client::cmd_join(const std::string &chat_name) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN;
        return;
    }
    std::string cmd = std::string("join ") + chat_name;
    this->main_pipe->put_message(cmd);
}

void Client::cmd_private(const std::string &login) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN;
        return;
    }
    std::string cmd = std::string("private ") + login;
    this->main_pipe->put_message(cmd);
}

void Client::cmd_create_chat(const std::string &login) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN;
        return;
    }
    std::string cmd = std::string("create ") + login;
    this->main_pipe->put_message(cmd);
}

void Client::cmd_push(const std::string &text) {
    if (this->state == NotLogin) {
        std::cout << ERROR_USER_IS_NOT_LOGGED_IN;
        return;
    }
    std::string cmd = std::string("push_to_") + ((this->state == InGroupChat) ? "public " : "private ") +
            this->chat_name + text;
    this->main_pipe->put_message(cmd);
}

void Client::set_main_pipe(const std::string &get_pipe, const std::string &put_pipe) {
    this->main_pipe = new FifoPipe(get_pipe, put_pipe);
}

