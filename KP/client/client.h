#ifndef KP_CLIENT_H
#define KP_CLIENT_H
#include <ctime>
#include "../structs/fifo_pipe.h"
#include "pthread.h"

enum ClientStates {NotLogin,InGroupChat,InPrivateChat,InMainMenu};
typedef void (*print_function)(const std::string&, int print_type, const std::string&);

class Client {
public:
    Client(print_function print);
    void start();
    void cmd_login(const std::string& login);
    void cmd_logout();
    void cmd_join(const std::string& chat_name);
    void cmd_private(const std::string& login);
    void cmd_create_chat(const std::string& chat_name);
    void cmd_push(const std::string& text);
    void cmd_chats();
    void rec_login(const std::string& args);
    void rec_join(const std::string& args);
    void rec_private(const std::string& args);
    void rec_create_chat(const std::string& args);
    void rec_chats(const std::string& args);
    void rec_get_msg(const std::string& args);
    ClientStates state;
    std::string last_update_id;
    void update_chat(const std::string& last_update_id);
    void print(const std::string& text, int print_type, const std::string& end="\n");
    void print(const char c, int print_type, const std::string& end="\n");
private:
    std::string login;
    std::string chat_name;
    FifoPipePut *request_pipe;
    FifoPipeGet *response_pipe;
    print_function print_f;
    void receive_loop();
    void answer_handler(const std::string& answer);
    void request(const std::string& command, const std::string& args);
};

#endif //KP_CLIENT_H
