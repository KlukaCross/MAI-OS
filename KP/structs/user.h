#ifndef KP_USER_H
#define KP_USER_H
#include <string>
#include <vector>
#include "fifo_pipe.h"
#include "user_message.h"

class User {
public:
    User(std::string& login, FifoPipe* pipe_worker);
    std::string login;
    std::vector<UserMessage> messages;
    FifoPipe* pipe_worker;
    bool authorized;
};

#endif //KP_USER_H
