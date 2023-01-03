#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include "constants.h"
#include "server.h"

int main() {
    auto *server = new Server();
    server->start();
    delete server;
    return 0;
}

Server::Server() {
    this->connected_pipe = new FifoPipe(CONNECTION_PIPE_NAME_IN, CONNECTION_PIPE_NAME_OUT);
}

Server::~Server() {
    free(this->connected_pipe);
}

void Server::start() {
    loop();
}

void Server::loop() {
    std::string request;
    while (true) {
        request = this->connected_pipe->get_message();
        request_handler(request, nullptr);
        for (auto & user : this->users) {
            request = user->pipe_worker->get_message();
            request_handler(request, user);
        }
        // todo: проверка, есть ли сообщение в пайпе
        // задержка
    }
}

void Server::request_handler(std::string& request, User* user) {
    // cmd args...
    int ind_to_cmd = request.find(' ');
    std::string cmd = request.substr(0, ind_to_cmd);
    if (cmd == "login") {
        std::string login = request.substr(ind_to_cmd+1, request.size()-ind_to_cmd);
        cmd_login(login);
    } else if (cmd == "logout") {
        cmd_disconnect(user);
    } else if (cmd == "private") {
        std::string login = request.substr(ind_to_cmd+1, request.size()-ind_to_cmd);
        cmd_private(login, user);
    } else if (cmd == "create") {

    }
}

FifoPipe *Server::gen_pipe() {
    return nullptr;
}

void Server::cmd_login(std::string& login) {
    std::string answer = "login ";
    bool err = false;
    User* user = nullptr;
    for (auto &u: this->users) {
        if (u->login == login) {
            user = u;
            if (u->authorized) {
                answer += std::string(STATUS_ERROR) + " user already logged in";
                err = true;
            }
            break;
        }
    }
    if (!err) {
        answer += STATUS_OK;
        auto fifo_pipe = gen_pipe();
        if (user == nullptr) {
            user = new User(login, fifo_pipe);
            this->users.push_back(user);
        } else {
            user->pipe_worker = fifo_pipe;
            user->authorized = true;
        }
        answer += std::string(fifo_pipe->get_pipe_name) + " " + fifo_pipe->put_pipe_name;
    }
    this->connected_pipe->put_message(answer);
}

void Server::cmd_disconnect(User *user) {
    delete user->pipe_worker;
    user->authorized = false;
}

void Server::cmd_private(std::string &login, User *user) {
    std::string answer = "private ";
    if (user->login == login) {
        answer += std::string(STATUS_ERROR) + " нельзя говорить с самим собой";
        this->connected_pipe->put_message(answer);
        return;
    }
    User* to_user = nullptr;
    for (auto &u: this->users) {
        if (u->login == login) {
            to_user = u;
            break;
        }
    }
    if (to_user == nullptr) {
        answer += std::string(STATUS_ERROR) + " user not found";
    } else {
        answer += std::string(STATUS_OK) + " " + login;
    }
    this->connected_pipe->put_message(answer);
}
