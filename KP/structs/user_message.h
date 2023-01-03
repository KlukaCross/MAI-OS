#ifndef KP_MESSAGE_H
#define KP_MESSAGE_H
#include <string>
#include <ctime>

class UserMessage {
public:
    UserMessage(std::string& from_login, std::string& text, std::time_t timestamp);
    std::string from_login;
    std::string text;
    std::string to_chat;
    std::time_t timestamp;
};

#endif //KP_MESSAGE_H
