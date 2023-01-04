#include "user_message.h"

UserMessage::UserMessage(const std::string& from_login, const std::string& text, std::time_t timestamp, unsigned long id) {
    this->from_login = from_login;
    this->text = text;
    this->timestamp = timestamp;
    this->id = id;
}
