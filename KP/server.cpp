#include <iostream>
#include "constants.h"
#include "server.h"
#include "funcs/string_parser.h"
#include <uuid/uuid.h>

int main() {
    auto *server = new Server();
    server->start();
    delete server;
    return 0;
}

const struct timespec SLEEP_TIME = {SECONDS_SLEEP, MILLISECONDS_SLEEP*1000000L}; // sec nanosec

Server::Server() {
    this->request_pipe = new FifoPipeGet(std::string(PIPE_PATH)+REQUEST_PIPE_NAME);
    this->message_counter_id = 0;
}

Server::~Server() {
    free(this->request_pipe);
}

void Server::start() {
    loop();
}

void Server::loop() {
    while (true) {
        std::string request;
        request = this->request_pipe->get_message();
        if (request.empty()) {
            nanosleep(&SLEEP_TIME, nullptr);
            continue;
        }
        std::cout << "get request " << request << std::endl;
        request_handler(request);
    }
}

void Server::request_handler(const std::string& request) {
    // cmd login args...
    std::vector<std::string> string_p;
    string_parser(request, 4, string_p, COMMAND_SEPARATOR);
    if (string_p.size() == 1) {
        std::cout << "Error: Bad request: " << request << std::endl;
        return;
    }
    std::string cmd = string_p[0];
    std::string user_pipe_name = string_p[1];
    std::string user_login = string_p[2];
    std::string args = (string_p.size() == 3)? "" : string_p[3];
    User* user = find_user(user_login);
    auto answer_pipe = FifoPipePut(user_pipe_name);
    if (cmd == CMD_LOGIN) {
        cmd_login(answer_pipe, args);
        return;
    }
    if (user == nullptr) {
        std::cout << "Error: User " << user_login << " not authorized" << std::endl;
        return;
    }
    if (cmd == CMD_LOGOUT) {
        cmd_logout(user);
    } else if (cmd == CMD_PRIVATE) {
        cmd_private(answer_pipe, user, args);
    } else if (cmd == CMD_CREATE) {
        cmd_create(answer_pipe, user, args);
    } else if (cmd == CMD_JOIN) {
        cmd_join(answer_pipe, user, args);
    } else if (cmd == CMD_PUSH_MSG) {
        std::vector<std::string> args_p;
        string_parser(args, 3, args_p, COMMAND_SEPARATOR);
        cmd_put_msg(answer_pipe, user, args_p[0], args_p[1], args_p[2]);
    } else if (cmd == CMD_GET_MSG) {
        std::vector<std::string> args_p;
        string_parser(args, 3, args_p, COMMAND_SEPARATOR);
        cmd_get_msg(answer_pipe, user, args_p[0], args_p[1], args_p[2]);
    } else if (cmd == CMD_CHATS) {
        cmd_chats(answer_pipe, user);
    }
}


void Server::cmd_login(FifoPipePut answer_pipe, const std::string& login) {
    std::string answer;
    std::string status;
    User* user = find_user(login);
    if (user != nullptr && user->authorized) {
        answer += "user " + login + " already logged in";
        status = STATUS_ERROR;
    }
    else {
        status += STATUS_OK;
        if (user == nullptr) {
            user = new User(login);
            this->users.push_back(user);
        } else {
            user->authorized = true;
        }
    }
    response(answer_pipe, CMD_LOGIN, status, answer);
}

void Server::cmd_logout(User* user) {
    user->authorized = false;
}

void Server::cmd_private(FifoPipePut answer_pipe, User* user, const std::string& login) {
    std::string answer;
    std::string status;
    if (user->login == login) {
        answer += "нельзя говорить с самим собой";
        status = STATUS_ERROR;
        response(answer_pipe, CMD_PRIVATE, status, answer);
        return;
    }
    User* to_user = find_user(login);
    if (to_user == nullptr) {
        answer += "user " + login + " not found";
        status = STATUS_ERROR;
    } else {
        PrivateChat *chat = nullptr;
        for (auto &c: this->private_chats) {
            if ((c->user1 == user && c->user2 == to_user) ||
                (c->user2 == user && c->user1 == to_user)) {
                chat = c;
                break;
            }
        }
        if (chat == nullptr) {
            auto pc = new PrivateChat(user, to_user);
            this->private_chats.push_back(pc);
        }
        answer += login;
        status = STATUS_OK;
    }
    response(answer_pipe, CMD_PRIVATE, status, answer);
}

void Server::cmd_create(FifoPipePut answer_pipe, User* user, const std::string &name) {
    std::string answer;
    std::string status;
    bool err = false;
    for (auto &c: this->group_chats) {
        if (c->name == name) {
            answer += "chat " + name + " already exists";
            status = STATUS_ERROR;
            err = true;
            break;
        }
    }
    if (!err) {
        auto gc = new GroupChat(name, user->login);
        this->group_chats.push_back(gc);
        gc->users.push_back(user);
        answer += name;
        status = STATUS_OK;
    }
    response(answer_pipe, CMD_CREATE, status, answer);
}

void Server::cmd_join(FifoPipePut answer_pipe, User *user, const std::string &name) {
    std::string answer;
    std::string status;
    GroupChat* gc = nullptr;
    for (auto &c: this->group_chats) {
        if (c->name == name) {
            gc = c;
            break;
        }
    }
    if (gc == nullptr) {
        answer += "chat " + name + " not found";
        status = STATUS_ERROR;
    } else {
        gc->users.push_back(user);
        answer += name;
        status = STATUS_OK;
    }
    response(answer_pipe, CMD_JOIN, status, answer);
}

void Server::response(FifoPipePut answer_pipe, const std::string &cmd, const std::string &status, const std::string &answer) {
    std::string ans;
    if (!answer.empty())
        ans += COMMAND_SEPARATOR;
    ans += answer;
    answer_pipe.put_message(cmd+COMMAND_SEPARATOR+status+ans);
}

User *Server::find_user(const std::string &user_login) {
    for (auto &u: this->users) {
        if (u->login == user_login) {
            return u;
        }
    }
    return nullptr;
}

void Server::cmd_get_msg(FifoPipePut answer_pipe, User *user, const std::string &mode, const std::string &name, const std::string &last_id) {
    std::list<UserMessage*> *msgs = nullptr;
    if (mode == ARG_GROUP) {
        GroupChat *chat = nullptr;
        for (auto &c: this->group_chats) {
            if (c->name == name) {
                chat = c;
                break;
            }
        }
        if (chat == nullptr) {
            response(answer_pipe, CMD_GET_MSG, STATUS_ERROR, "chat " + name + " not found");
            return;
        }
        msgs = &chat->messages;
    } else if (mode == ARG_PRIVATE) {
        PrivateChat *chat = nullptr;
        for (auto &c: this->private_chats) {
            if ((c->user1 == user && c->user2->login == name) || (
                    c->user2 == user && c->user1->login == name)) {
                chat = c;
                break;
            }
        }
        if (chat == nullptr) {
            response(answer_pipe, CMD_GET_MSG, STATUS_ERROR, "user " + name + " not found");
            return;
        }
        msgs = &chat->messages;
    }
    std::string answer;
    unsigned long id = std::strtol(last_id.data(), nullptr, 10);
    unsigned long lid = id;
    for (auto &m: *msgs) {
        if (m->id > id) {
            answer += m->from_login + ": " + m->text + COMMAND_SEPARATOR;
            lid = std::max(lid, m->id);
        }
    }
    answer = std::to_string(lid) + COMMAND_SEPARATOR + answer;
    response(answer_pipe, CMD_GET_MSG, STATUS_OK, answer);
}

void Server::cmd_put_msg(FifoPipePut answer_pipe, User *user, const std::string &mode, const std::string &name, const std::string &text) {
    std::list<UserMessage*> *msgs = nullptr;
    if (mode == ARG_GROUP) {
        GroupChat *chat = nullptr;
        for (auto &c: this->group_chats) {
            if (c->name == name) {
                chat = c;
                break;
            }
        }
        if (chat == nullptr) {
            response(answer_pipe, CMD_GET_MSG, STATUS_ERROR, "chat " + name + " not found");
            return;
        }
        msgs = &chat->messages;
    } else if (mode == ARG_PRIVATE) {
        PrivateChat *chat = nullptr;
        for (auto &c: this->private_chats) {
            if (c->user1 == user || c->user2 == user) {
                chat = c;
                break;
            }
        }
        if (chat == nullptr) {
            response(answer_pipe, CMD_GET_MSG, STATUS_ERROR, "user " + name + " not found");
            return;
        }
        msgs = &chat->messages;
    }

    auto um = new UserMessage(user->login, text, std::time(nullptr), ++this->message_counter_id);
    this->messages.push_back(um);
    (*msgs).push_back(um);
    response(answer_pipe, CMD_PUSH_MSG, STATUS_OK, "");
}

void Server::cmd_chats(FifoPipePut answer_pipe, User *user) {
    std::string answer;
    for (auto &c: this->private_chats) {
        std::string other_name;
        if (c->user1 == user)
            other_name = c->user2->login;
        if (c->user2 == user)
            other_name = c->user1->login;
        if (!other_name.empty())
            answer += other_name + ((c->messages.empty())? "" : " " + c->messages.back()->text) + COMMAND_SEPARATOR;
    }
    for (auto &c: this->group_chats) {
        bool fl = false;
        for (auto &u: c->users) {
            if (u == user) {
                fl = true;
                break;
            }
        }
        if (fl)
            answer += c->name + ((c->messages.empty())? "" : " " + c->messages.back()->from_login + ": " + c->messages.back()->text) +
                    COMMAND_SEPARATOR;
    }
    response(answer_pipe, CMD_CHATS, STATUS_OK, answer);
}
