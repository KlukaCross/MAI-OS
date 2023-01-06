#include "client/client.h"
#include <pthread.h>
#include <signal.h>
#include <vector>
#include "funcs/string_parser.h"
#include "constants.h"
#include "tui/tui.h"

void* client_receiver(void* args);
void* client_chat_updater(void* args);
void input_handler(std::string& text);
void update_chat(std::string& chat_name, bool is_public);
void back_to_menu();
void print_help();
void print(const std::string& text, int print_type, const std::string& end="\n");

const struct timespec SLEEP_TIME = {UPDATE_CHAT_SECONDS_SLEEP, UPDATE_CHAT_MILLISECONDS_SLEEP * 1000000L}; // sec nanosec

pthread_t client_receiver_p;
pthread_t client_updater_p;
Client *client = nullptr;
TUI *tui = nullptr;

int main() {
    setlocale(LC_ALL, "Rus");

    client = new Client(print);
    tui = new TUI(update_chat, input_handler, print_help, back_to_menu);

    signal(SIGPIPE, SIG_IGN);

    pthread_create(&client_receiver_p, nullptr, client_receiver, client);
    pthread_create(&client_updater_p, nullptr, client_chat_updater, client);
    tui->start();
    pthread_cancel(client_updater_p);
    pthread_join(client_updater_p, nullptr);
    pthread_cancel(client_receiver_p); // возможно, нужна задержка, чтобы получить последнее сообщение
    pthread_join(client_receiver_p, nullptr);
    if (client->state != NotLogin)
        client->cmd_logout();
    delete client;
    return 0;
}

void back_to_menu() {
    if (client->state == InMainMenu || client->state == NotLogin)
        return;
    client->state = InMainMenu;
    tui->clear_output();
    print_help();
}

void print(const std::string& text, int print_type, const std::string& end) {
    if (print_type == PRINT_SYSTEM_INFO_OK)
        tui->output(text+end, TUI_COLOR_MAGENTA);
    else if (print_type == PRINT_SYSTEM_INFO_ERR)
        tui->output(text+end, TUI_COLOR_RED);
    else if (print_type == PRINT_MESSAGE) { // text=user|message|...
        int state_now = 0; // 0 - user, 1 - message
        int last_sep = -1;
        for (int i = 0; i < text.size(); ++i) {
            if (text[i] != COMMAND_ARGS_SEPARATOR)
                continue;
            if (state_now == 0) {
                std::string s = text.substr(last_sep+1, i-last_sep-1);
                tui->output(s, TUI_COLOR_BLUE);
                tui->output(": ", TUI_COLOR_WHITE);
                state_now++;
            } else {
                std::string s = text.substr(last_sep+1, i-last_sep-1) + '\n';
                tui->output(s, TUI_COLOR_WHITE);
                state_now = 0;
            }
            last_sep = i;
        }
        std::string s = text.substr(last_sep+1, text.size()-last_sep-1) + '\n';
        tui->output(s, TUI_COLOR_WHITE);
    } else if (print_type == PRINT_CHAT_INFO) {
        std::vector<std::string> chats;
        int last_sep = -1;
        for (int i = 0; i < text.size(); ++i) {
            if (text[i] != COMMAND_ARGS_SEPARATOR)
                continue;
            chats.push_back(text.substr(last_sep+1, i-last_sep-1));
            last_sep = i;
        }
        chats.push_back(text.substr(last_sep+1, text.size()-last_sep-1));
        tui->update_chats(chats);
    }
}

void print_help() {
    print("F1 - print help", PRINT_SYSTEM_INFO_OK);
    print("F12 - exit", PRINT_SYSTEM_INFO_OK);
    if (client->state == NotLogin) {
        print(std::string(CMD_LOGIN) + " LOGIN - log in under the login LOGIN", PRINT_SYSTEM_INFO_OK);
    } else if (client->state == InMainMenu) {
        print("Main menu", PRINT_SYSTEM_INFO_OK);
        print(std::string(CMD_CHATS) + " - view the list of chats in which there was communication", PRINT_SYSTEM_INFO_OK);
        print(std::string(CMD_PRIVATE) + " LOGIN - log in to a personal chat with a LOGIN user", PRINT_SYSTEM_INFO_OK);
        print(std::string(CMD_JOIN) + " CHAT_NAME - log in to a group chat with the name CHAT_NAME", PRINT_SYSTEM_INFO_OK);
        print(std::string(CMD_CREATE) + " CHAT_NAME - create a group chat with the name CHAT_NAME", PRINT_SYSTEM_INFO_OK);
        print(std::string(CMD_LOGOUT) + " - log out of the system", PRINT_SYSTEM_INFO_OK);
    } else {
        print("F2 - go back to the menu", PRINT_SYSTEM_INFO_OK);
    }
}

void* client_receiver(void* args) {
    client->start();
    return nullptr;
}

void* client_chat_updater(void* args) {
    while (true) {
        if (client->state == NotLogin) {
            nanosleep(&SLEEP_TIME, nullptr);
            continue;
        }
        if (client->state == InPrivateChat || client->state == InGroupChat)
            client->update_chat(client->last_update_id);
        nanosleep(&SLEEP_TIME, nullptr);
        client->cmd_chats();
        nanosleep(&SLEEP_TIME, nullptr);
    }
    return nullptr;
}

void input_handler(std::string& text) {
    if (text.empty())
        return;
    if (client->state == InPrivateChat || client->state == InGroupChat) {
        client->cmd_push(text);
        return;
    }
    std::vector<std::string> args_p;
    string_parser(text, 2, args_p, ' ');
    std::string cmd = args_p[0];
    if (cmd == CMD_HELP)
        print_help();
    else if (cmd == CMD_LOGIN) {
        client->cmd_login(args_p[1]);
    } else if (cmd == CMD_LOGOUT) {
        tui->clear_output();
        tui->clear_chats();
        client->cmd_logout();
    }
    else if (cmd == CMD_PRIVATE) {
        client->cmd_private(args_p[1]);
    } else if (cmd == CMD_JOIN) {
        client->cmd_join(args_p[1]);
    } else if (cmd == CMD_CREATE) {
        client->cmd_create_chat(args_p[1]);
    } else if (cmd == CMD_CHATS) {
        client->cmd_chats();
    } else
        print("Unknown command " + cmd, PRINT_SYSTEM_INFO_ERR);
}

void update_chat(std::string& chat_name, bool is_public) {
    tui->clear_output();
    if (is_public)
        client->cmd_join(chat_name);
    else
        client->cmd_private(chat_name);
}
