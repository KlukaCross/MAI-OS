#ifndef KP_CHAT_BUFFER_H
#define KP_CHAT_BUFFER_H
#include <vector>
#include <string>

struct ChatBufElem {
    bool is_public;
    std::string chat_name;
    int chat_name_color;
    std::string last_user_name;
    int last_user_name_color;
    std::string last_message;
    int last_message_color;
};

class ChatBuffer {
public:
    ChatBufElem* add(bool is_public, const std::string& chat_name, int chat_name_color, const std::string& last_user_name, int last_user_name_color,
             const std::string& last_message, int last_message_color);
    ChatBufElem* get(const std::string& chat_name);
    void clear();
    std::vector<ChatBufElem*>::iterator begin();
    std::vector<ChatBufElem*>::iterator end();
    bool now_choice_up();
    bool now_choice_down();
    ChatBufElem* now_choice_get();
    void set_now_choice(int pos);
    bool empty();
    ~ChatBuffer();
private:
    int now_choice;
    std::vector<ChatBufElem*> buffer;
};

#endif //KP_CHAT_BUFFER_H
