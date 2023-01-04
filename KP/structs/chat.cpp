#include "chat.h"

GroupChat::GroupChat(const std::string& name, std::string& owner_name) {
    this->name = name;
    this->owner_name = owner_name;
}

PrivateChat::PrivateChat(const User *user1, const User *user2) {
    this->user1 = user1;
    this->user2 = user2;
}
