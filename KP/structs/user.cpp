#include "user.h"

User::User(std::string &login, FifoPipe *pipe_worker) {
    this->login = login;
    this->pipe_worker = pipe_worker;
    this->authorized = true;
}
