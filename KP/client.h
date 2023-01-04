#ifndef KP_CLIENT_H
#define KP_CLIENT_H
#include <ctime>
#include "structs/fifo_pipe.h"
#include "pthread.h"

enum ClientStates {NotLogin,InGroupChat,InPrivateChat,InMainMenu};

class Client {
public:
    Client();
    void start();
    void cmd_login(const std::string& login);
    void cmd_logout();
    void cmd_join(const std::string& chat_name);
    void cmd_private(const std::string& login);
    void cmd_create_chat(const std::string& chat_name);
    void cmd_push(const std::string& text);
    void cmd_chats();
    ClientStates state;
    std::string last_update_id;
    void update_chat(const std::string& last_update_id);
private:
    std::string login;
    std::string chat_name;
    FifoPipePut *request_pipe;
    FifoPipeGet *response_pipe;
    void receive_loop();
    void answer_handler(const std::string& answer);
    void request(const std::string& command, const std::string& args);
};

#endif //KP_CLIENT_H
