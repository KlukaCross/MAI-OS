#ifndef KP_GROUP_CHATS_H
#define KP_GROUP_CHATS_H
#include <string>
#include <list>

class GroupChat {
public:
    GroupChat(std::string& name, std::string& owner_name);
    std::string name;
    std::string owner_name;
    std::list<std::string> user_logins;
};

#endif //KP_GROUP_CHATS_H
