#ifndef KP_USER_H
#define KP_USER_H
#include <string>
#include <vector>
#include "fifo_pipe.h"
#include "user_message.h"

class User {
public:
    User(const std::string& login);
    std::string login;
    bool authorized;
    // неплохо бы еще хранить вектор чатов, к которым присоединен этот юзер
};

#endif //KP_USER_H
