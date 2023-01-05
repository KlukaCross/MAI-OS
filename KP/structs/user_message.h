#ifndef KP_MESSAGE_H
#define KP_MESSAGE_H
#include <string>
#include <ctime>

class Message {
public:
    Message(const std::string& from_login, const std::string& text, std::time_t timestamp, unsigned long id);
    std::string from_login;
    std::string text;
    std::time_t timestamp;
    unsigned long id;
};

#endif //KP_MESSAGE_H
