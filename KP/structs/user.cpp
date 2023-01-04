#include "user.h"

User::User(const std::string &login) {
    this->login = login;
    this->authorized = true;
}
