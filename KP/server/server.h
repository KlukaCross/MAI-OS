#ifndef KP_SERVER_H
#define KP_SERVER_H
#include <pthread.h>
#include <list>
#include "../structs/user.h"
#include "../structs/chat.h"
#include "../structs/fifo_pipe.h"

class Server {
public:
    Server();
    void start();
    ~Server();
private:
    void loop();
    FifoPipeGet *request_pipe;
    std::list<User*> users;
    std::list<GroupChat*> group_chats;
    std::list<PrivateChat*> private_chats;
    std::list<Message*> messages;
    void request_handler(const std::string& request);
    void cmd_login(FifoPipePut answer_pipe, const std::string& login);
    void cmd_logout(User* user);
    void cmd_private(FifoPipePut answer_pipe, User* user, const std::string& login);
    void cmd_create(FifoPipePut answer_pipe, User* user, const std::string& name);
    void cmd_join(FifoPipePut answer_pipe, User* user, const std::string& name);
    void cmd_get_msg(FifoPipePut answer_pipe, User* user, const std::string& mode, const std::string& name, const std::string& last_id);
    void cmd_put_msg(FifoPipePut answer_pipe,  User* user, const std::string& mode, const std::string& name, const std::string& text);
    void cmd_chats(FifoPipePut answer_pipe, User* user);
    User* find_user(const std::string& user_login);
    void response(FifoPipePut answer_pipe, const std::string& cmd, const std::string& status, const std::string& answer);
    unsigned long message_counter_id;
};

#endif //KP_SERVER_H
