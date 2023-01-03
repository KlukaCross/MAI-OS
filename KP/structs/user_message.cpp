#include "user_message.h"

UserMessage::UserMessage(std::string& from_login, std::string& text, std::time_t timestamp) {
    this->from_login = from_login;
    this->text = text;
    this->timestamp = timestamp;
}
