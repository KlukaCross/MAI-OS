#ifndef KP_GROUP_CHATS_H
#define KP_GROUP_CHATS_H
#include "user.h"
#include <string>
#include <list>

class Chat {
public:
    std::list<UserMessage*> messages;
};

class GroupChat : public Chat {
public:
    GroupChat(const std::string& name, std::string& owner_name);
    std::string name;
    std::string owner_name;
    std::list<User*> users;
};

class PrivateChat : public Chat {
public:
    PrivateChat(const User* user1, const User* user2);
    const User* user1;
    const User* user2;
};

#endif //KP_GROUP_CHATS_H
