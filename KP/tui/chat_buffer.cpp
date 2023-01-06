#include "chat_buffer.h"

ChatBufElem* ChatBuffer::add(bool is_public, const std::string &chat_name, int chat_name_color, const std::string &last_user_name,
                     int last_user_name_color, const std::string &last_message, int last_message_color) {
    auto buf = new ChatBufElem();
    buf->is_public = is_public;
    buf->chat_name = chat_name;
    buf->chat_name_color = chat_name_color;
    buf->last_user_name = last_user_name;
    buf->last_user_name_color = last_user_name_color;
    buf->last_message = last_message;
    buf->last_message_color = last_message_color;
    this->buffer.push_back(buf);
    return buf;
}

ChatBufElem *ChatBuffer::get(const std::string &chat_name) {
    for (auto &el: this->buffer) {
        if (el->chat_name == chat_name)
            return el;
    }
    return nullptr;
}

void ChatBuffer::clear() {
    for (auto &el: this->buffer) {
        delete el;
    }
    this->now_choice = 0;
    this->buffer.clear();
}

ChatBuffer::~ChatBuffer() {
    this->clear();
}

std::vector<ChatBufElem *>::iterator ChatBuffer::begin() {
    return this->buffer.begin();
}

std::vector<ChatBufElem *>::iterator ChatBuffer::end() {
    return this->buffer.end();
}

bool ChatBuffer::now_choice_up() {
    if (this->now_choice <= 0)
        return false;
    this->now_choice--;
    return true;
}

bool ChatBuffer::now_choice_down() {
    if (this->now_choice >= this->buffer.size()-1)
        return false;
    this->now_choice++;
    return true;
}

ChatBufElem *ChatBuffer::now_choice_get() {
    return this->buffer.at(this->now_choice);
}

void ChatBuffer::set_now_choice(int pos) {
    this->now_choice = pos;
}

bool ChatBuffer::empty() {
    return this->buffer.empty();
}
