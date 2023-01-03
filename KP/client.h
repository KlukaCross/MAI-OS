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
    void cmd_create_chat(const std::string& login);
    void cmd_push(const std::string& text);
    void set_main_pipe(const std::string& get_pipe, const std::string& put_pipe);
    ClientStates state;
private:
    std::string login;
    std::string chat_name;
    FifoPipe *authorization_pipe;
    FifoPipe *main_pipe;
    std::time_t last_update_timestamp;
    void loop();
    void update_group_chat(std::time_t timestamp);
    void update_private_chat(std::time_t timestamp);
    void answer_handler(const std::string& answer);
};

#endif //KP_CLIENT_H
