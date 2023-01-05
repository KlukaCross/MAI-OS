#include <iostream>
#include <pthread.h>
#include <signal.h>
#include "constants.h"
#include "client.h"
#include "funcs/string_parser.h"
#include "funcs/gen_uuid.h"
#define ERROR_USER_IS_NOT_LOGGED_IN "user is not logged in"

void* console_reader(void* args);
void* client_receiver(void* args);
void* client_updater(void* args);
void back_to_menu(int signal);

const struct timespec SLEEP_TIME = {SECONDS_SLEEP, MILLISECONDS_SLEEP*1000000L}; // sec nanosec

bool IN_MENU = false;
pthread_t console_reader_p;
pthread_t client_receiver_p;
pthread_t client_updater_p;

int main() {
    auto *client = new Client();
    struct sigaction new_action{}, old_action{};
    new_action.sa_handler = back_to_menu;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction (SIGINT, nullptr, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction (SIGINT, &new_action, nullptr);
    signal(SIGPIPE, SIG_IGN);
    IN_MENU = true;
    pthread_create(&console_reader_p, nullptr, console_reader, client);
    pthread_create(&client_receiver_p, nullptr, client_receiver, client);
    pthread_create(&client_updater_p, nullptr, client_updater, client);
    pthread_join(console_reader_p, nullptr);
    pthread_cancel(client_updater_p);
    pthread_join(client_updater_p, nullptr);
    pthread_cancel(client_receiver_p); // возможно, нужна задержка, чтобы получить последнее сообщение
    pthread_join(client_receiver_p, nullptr);
    if (client->state != NotLogin)
        client->cmd_logout();
    delete client;
    return 0;
}

void back_to_menu(int signal) {
    if (IN_MENU)
        pthread_cancel(console_reader_p);
    IN_MENU = true;
}

void print(const std::string& text, const std::string& end="\n") {
    std::cout << text << end;
}
void print(char text, const std::string& end="\n") {
    std::string a;
    a += text;
    print(a, end);
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
            if (IN_MENU) {
                client->state = InMainMenu;
                print("Main menu");
                print_help(client->state);
            }
            if (!text.empty())
                client->cmd_push(text);
            continue;
        }
        if (std::cin.eof())
            pthread_cancel(console_reader_p);
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
        } else
            print("Unknown command " + cmd);
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
        std::string answer = this->response_pipe->get_message(COMMAND_SEPARATOR);
        if (answer.empty()) {
            nanosleep(&SLEEP_TIME, nullptr);
            continue;
        }
        answer_handler(answer);
    }
}

void Client::update_chat(const std::string& last_update_id) {
    std::string args = std::string((this->state == InGroupChat) ? ARG_GROUP : ARG_PRIVATE) + COMMAND_ARGS_SEPARATOR +
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
        print("Error: " + args);
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
        print("user is already logged in");
        return;
    }
    if (login.size() < MIN_NAME_SIZE || login.size() > MAX_NAME_SIZE) {
        print("Name size must be greater than " + std::to_string(MIN_NAME_SIZE-1) + " and less than " +
            std::to_string(MAX_NAME_SIZE+1));
        return;
    }
    if (login.find(COMMAND_ARGS_SEPARATOR) != -1) {
        print("Name contains forbidden characters");
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
    if (chat_name.size() < MIN_NAME_SIZE || chat_name.size() > MAX_NAME_SIZE) {
        print("Chat name size must be greater than " + std::to_string(MIN_NAME_SIZE-1) + " and less than " +
              std::to_string(MAX_NAME_SIZE+1));
        return;
    }
    if (chat_name.find(COMMAND_ARGS_SEPARATOR) != -1) {
        print("Chat name contains forbidden characters");
        return;
    }
    request(CMD_CREATE, chat_name);
}

void Client::cmd_push(const std::string &text) {
    if (this->state == NotLogin) {
        print(ERROR_USER_IS_NOT_LOGGED_IN);
        return;
    }
    std::string req = text;
    if (text.find(COMMAND_ARGS_SEPARATOR) != -1) {
        print("Text contains forbidden characters"); // лучше использовать экранирование
        return;
    }
    if (text.size() > MAX_MESSAGE_CHARACTERS) {
        print("Message size must be less than " + std::to_string(MAX_MESSAGE_CHARACTERS+1));
        print("The message is truncated");
        req = req.substr(0, MAX_MESSAGE_CHARACTERS);
    }
    std::string args = std::string((this->state == InGroupChat) ? ARG_GROUP : ARG_PRIVATE) + COMMAND_ARGS_SEPARATOR + chat_name + COMMAND_ARGS_SEPARATOR + req;
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
    print("successful authorization");
    print_help(this->state);
}

void Client::rec_join(const std::string &args) {
    this->state = InGroupChat;
    IN_MENU = false;
    this->chat_name = args;
    this->last_update_id = '0';
    print("Group chat " + args);
}

void Client::rec_private(const std::string &args) {
    this->state = InPrivateChat;
    IN_MENU = false;
    this->chat_name = args;
    this->last_update_id = '0';
    print("Chat with the user " + args);
}

void Client::rec_create_chat(const std::string &args) {
    this->state = InGroupChat;
    IN_MENU = false;
    this->chat_name = args;
    this->last_update_id = '0';
    print("Group chat " + args);
}

void Client::rec_chats(const std::string &args) {
    // chat_name1|user_name1|last_message1|chat_name2|user_name2|last_message2|...
    if (args.empty()) {
        print("No chats");
        return;
    }
    int state_now = 0; // 0 - chat, 1 - user, 2 - message
    for (const char &i: args) {
        if (i != COMMAND_ARGS_SEPARATOR) {
            print(i, "");
            continue;
        }
        if (state_now == 0) {
            print("\t", "");
            state_now++;
        } else if (state_now == 1) {
            print(": ", "");
            state_now++;
        } else {
            print("");
            state_now = 0;
        }
    }
    print("");
}

void Client::rec_get_msg(const std::string &args) {
    // user_name1|message1|user_name2|message2|...
    std::vector<std::string> args_p;
    string_parser(args, 2, args_p, COMMAND_ARGS_SEPARATOR);
    this->last_update_id = args_p[0];
    if (args_p[1].empty())
        return;
    int state_now = 0; // 0 - user, 1 - message
    for (char & i : args_p[1]) {
        if (i != COMMAND_ARGS_SEPARATOR) {
            print(i, "");
            continue;
        }
        if (state_now == 0) {
            print(": ", "");
            state_now++;
        } else {
            print("");
            state_now = 0;
        }
    }
    print("");
}