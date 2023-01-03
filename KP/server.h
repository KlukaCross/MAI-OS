#ifndef KP_SERVER_H
#define KP_SERVER_H
#include <pthread.h>
#include <list>
#include "structs/user.h"
#include "structs/group_chat.h"
#include "structs/fifo_pipe.h"

class Server {
public:
    Server();
    void start();
    ~Server();
private:
    void loop();
    FifoPipe *connected_pipe;
    std::list<User*> users;
    std::list<GroupChat*> group_chats;
    void request_handler(std::string& request, User* user);
    void cmd_login(std::string& login);
    void cmd_disconnect(User* user);
    void cmd_private(std::string& login, User* user);
    FifoPipe* gen_pipe();
};

#endif //KP_SERVER_H
