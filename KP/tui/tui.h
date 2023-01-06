#ifndef KP_TUI_H
#define KP_TUI_H
#include <panel.h>
#include <string>
#include <vector>
#include <signal.h>
#include "text_buffer.h"
#include "chat_buffer.h"
#include "../constants.h"

typedef void (*callback_update_list_position)(std::string&, bool);
typedef void (*callback_input)(std::string&);
typedef void (*callback_noargs)();

struct cursed_window {
    WINDOW *decoration;
    WINDOW *overlay;
    PANEL *panel;
};

class TUI {
public:
    TUI(callback_update_list_position callback_ulp, callback_input input, callback_noargs callback_help, callback_noargs callback_back);
    void start();
    void output(const std::string &s, int color);
    void clear_output();
    void update_chats(const std::vector<std::string>& chats);
    void clear_chats();
    void update_windows();
private:
    void loop();
    cursed_window* output_window;
    cursed_window* input_window;
    cursed_window* chats_window;
    cursed_window* now_window;
    ChatBuffer chat_list;
    TextBuffer output_text;
    std::string input_text;
    callback_update_list_position callback_ulp;
    callback_input callback_inp;
    callback_noargs callback_help;
    callback_noargs callback_back_to_menu;
    cursed_window *new_window(int sy, int sx, int h, int w);
    void resize_window(cursed_window* cw, int sy, int sx, int h, int w);
    void print_chats();
    void print_output_text(TextBufElem* text);
    void fix_cursor();
    void toggle_window();
};

#endif //KP_TUI_H
