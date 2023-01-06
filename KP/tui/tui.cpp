#include "tui.h"

TUI::TUI(callback_update_list_position callback_ulp, callback_input callback_inp, callback_noargs callback_help, callback_noargs callback_back) {
    this->chat_list = ChatBuffer();
    this->output_text = TextBuffer();
    this->callback_ulp = callback_ulp;
    this->callback_inp = callback_inp;
    this->callback_help = callback_help;
    this->callback_back_to_menu = callback_back;

    initscr();
    cbreak();
    //raw();
    //nonl();
    timeout(TUI_DELAY_MS);
    noecho();
    curs_set(1);
    keypad(stdscr, TRUE);
    if (has_colors() == FALSE)
    {
        endwin();
        puts("\nYour terminal does not support color");
    }
    start_color();
    use_default_colors();
    init_pair(TUI_COLOR_WHITE, COLOR_WHITE, TUI_MAIN_COLOR_BACK);
    init_pair(TUI_COLOR_BLUE, COLOR_BLUE, TUI_MAIN_COLOR_BACK);
    init_pair(TUI_COLOR_MAGENTA, COLOR_MAGENTA, TUI_MAIN_COLOR_BACK);
    init_pair(TUI_COLOR_RED, COLOR_RED, TUI_MAIN_COLOR_BACK);
    init_pair(TUI_COLOR_GREEN, COLOR_GREEN, TUI_MAIN_COLOR_BACK);

    int max_h, max_v;
    getmaxyx(stdscr, max_v, max_h);
    int output_window_h = max_v - max_v / TUI_VERTICAL_RATIO;
    int output_window_w = max_h - max_h / TUI_HORIZONTAL_RATIO;
    this->output_window = new_window(0, 0, output_window_h, output_window_w);
    this->input_window = new_window(output_window_h, 0, max_v / TUI_VERTICAL_RATIO, output_window_w);
    this->chats_window = new_window(0, output_window_w, max_v, max_h / TUI_HORIZONTAL_RATIO);
    this->now_window = input_window;

    fix_cursor();
    update_panels();
    doupdate();
}

cursed_window *TUI::new_window(int sy, int sx, int h, int w) {
    auto *curw = new cursed_window();
    curw->decoration = newwin(h, w, sy, sx);
    wbkgd(curw->decoration, COLOR_PAIR(TUI_COLOR_WHITE));
    box(curw->decoration, 0, 0);
    int x, y;
    getmaxyx(curw->decoration, y, x);
    curw->overlay = derwin(curw->decoration,  y-2, x-2, 1, 1);
    wbkgd(curw->overlay, COLOR_PAIR(TUI_COLOR_WHITE));
    scrollok(curw->overlay, TRUE);
    curw->panel = new_panel(curw->decoration);
    update_panels();
    doupdate();
    return curw;
}

void TUI::start() {
    loop();
}

void TUI::loop() {
    int user_key;
    this->callback_help();
    while (true)
    {
        user_key = getch();
        if (user_key != ERR) {
            if (user_key == TUI_KEY_EXIT)
                break;
            switch ( user_key )
            {
                case KEY_RESIZE:
                    update_windows();
                    fix_cursor();
                    break;
                case TUI_KEY_HELP:
                    this->callback_help();
                    fix_cursor();
                    break;
                case TUI_KEY_BACK_TO_MENU:
                    this->callback_back_to_menu();
                    break;
                case '\t': //TAB
                    toggle_window();
                    break;
                case '\n':
                    if (this->now_window == this->input_window) {
                        wclear(this->input_window->overlay);
                        wnoutrefresh(this->input_window->overlay);
                        this->callback_inp(this->input_text);
                        this->input_text.clear();
                        fix_cursor();
                    } else if (this->now_window == this->chats_window && !this->chat_list.empty()) {
                        callback_ulp(this->chat_list.now_choice_get()->chat_name, this->chat_list.now_choice_get()->is_public);
                    }
                    break;
                case KEY_DOWN:
                    if (this->now_window != this->chats_window)
                        break;
                    if (!this->chat_list.now_choice_down())
                        break;
                    wclear(this->chats_window->overlay);
                    print_chats();
                    break;
                case KEY_UP:
                    if (this->now_window != this->chats_window)
                        break;
                    if (!this->chat_list.now_choice_up())
                        break;
                    wclear(this->chats_window->overlay);
                    print_chats();
                    break;
                case KEY_BACKSPACE:
                    if (this->now_window == this->input_window) {
                        int wx = getmaxx(this->input_window->overlay);
                        int cy, cx;
                        getyx(this->input_window->overlay, cy, cx);
                        mvwdelch(this->input_window->overlay, (cx>0) ? cy : (cy - 1), (cx>0) ? (cx - 1) : wx-1);
                        if (!this->input_text.empty())
                            this->input_text.pop_back();
                        wnoutrefresh(this->input_window->overlay);
                    }
                    break;
                default:
                    if (this->now_window == this->input_window) {
                        waddch(this->input_window->overlay, user_key);
                        this->input_text += char(user_key);
                        wnoutrefresh(this->input_window->overlay);
                    }
                    break;
            }
        }

        doupdate();
    }
    del_panel(this->input_window->panel);
    del_panel(this->output_window->panel);
    del_panel(this->chats_window->panel);
    delwin(this->input_window->decoration);
    delwin(this->output_window->decoration);
    delwin(this->chats_window->decoration);
    delwin(this->input_window->overlay);
    delwin(this->output_window->overlay);
    delwin(this->chats_window->overlay);
    endwin();
}

void TUI::update_windows() {
    int max_h, max_v;
    getmaxyx(stdscr, max_v, max_h);
    int output_window_h = max_v - max_v / TUI_VERTICAL_RATIO;
    int output_window_w = max_h - max_h / TUI_HORIZONTAL_RATIO;
    resize_window(this->output_window, 0, 0, output_window_h, output_window_w);
    resize_window(this->input_window, output_window_h, 0, max_v / TUI_VERTICAL_RATIO, output_window_w);
    resize_window(this->chats_window, 0, output_window_w, max_v, max_h / TUI_HORIZONTAL_RATIO);
    move_panel(this->input_window->panel, output_window_h, 0);
    move_panel(this->chats_window->panel, 0, output_window_w);

    for (auto & i : this->output_text) {
        print_output_text(i);
    }
    if (!this->input_text.empty())
        waddstr(this->input_window->overlay, this->input_text.c_str());
    if (!this->chat_list.empty())
        print_chats();

    update_panels();
    doupdate();
}

void TUI::resize_window(cursed_window *cw, int sy, int sx, int h, int w) {
    WINDOW* new_dec = newwin(h, w, sy, sx);
    box(new_dec, 0, 0);
    wbkgd(new_dec, COLOR_PAIR(TUI_COLOR_WHITE));

    int x, y;
    getmaxyx(new_dec, y, x);
    WINDOW* new_over = derwin(new_dec, y-2, x-2, 1, 1);
    wbkgd(new_over, COLOR_PAIR(TUI_COLOR_WHITE));
    scrollok(new_over, TRUE);

    replace_panel(cw->panel, new_dec);
    delwin(cw->decoration);
    delwin(cw->overlay);
    cw->decoration = new_dec;
    cw->overlay = new_over;
}

void TUI::output(const std::string &s, int color) {
    auto el = this->output_text.add(s, color);
    print_output_text(el);
}

void TUI::clear_output() {
    this->output_text.clear();
    wclear(this->output_window->overlay);
}

void TUI::print_chats() {
    if (this->chat_list.empty())
        return;
    auto now_chat = this->chat_list.now_choice_get();
    for (auto &chat: this->chat_list) {
        if (chat == now_chat)
            waddch(this->chats_window->overlay, TUI_CHAT_CHOICE_CHAR);
        else
            waddch(this->chats_window->overlay, TUI_CHAT_NOCHOICE_CHAR);
        wattron(this->chats_window->overlay, COLOR_PAIR(chat->chat_name_color));
        waddstr(this->chats_window->overlay, chat->chat_name.c_str());
        wattroff(this->chats_window->overlay, COLOR_PAIR(chat->chat_name_color));
        waddstr(this->chats_window->overlay, TUI_CHAT_SEPARATOR_1);
        wattron(this->chats_window->overlay, COLOR_PAIR(chat->last_user_name_color));
        waddstr(this->chats_window->overlay, chat->last_user_name.c_str());
        wattroff(this->chats_window->overlay, COLOR_PAIR(chat->last_user_name_color));
        waddstr(this->chats_window->overlay, TUI_CHAT_SEPARATOR_2);
        wattron(this->chats_window->overlay, COLOR_PAIR(chat->last_message_color));
        waddstr(this->chats_window->overlay, chat->last_message.c_str());
        wattroff(this->chats_window->overlay, COLOR_PAIR(chat->last_message_color));
        waddstr(this->chats_window->overlay, TUI_CHAT_SEPARATOR_3);
    }
    wnoutrefresh(this->chats_window->overlay);
    fix_cursor();
}

void TUI::print_output_text(TextBufElem *text) {
    wattron(this->output_window->overlay, COLOR_PAIR(text->color));
    waddstr(this->output_window->overlay, text->text.c_str());
    wattroff(this->output_window->overlay, COLOR_PAIR(text->color));
    fix_cursor();
    wnoutrefresh(this->output_window->overlay);
}

void TUI::clear_chats() {
    this->chat_list.clear();
    wclear(this->chats_window->overlay);
}

void TUI::update_chats(const std::vector<std::string>& chats) {
    if (chats.empty())
        return;
    if (!this->chat_list.empty() && chats.size() >= 4 &&
        chats[1] == (*this->chat_list.begin())->chat_name &&
        chats[2] == (*this->chat_list.begin())->last_user_name &&
        chats[3] == (*this->chat_list.begin())->last_message) {
        return; // dont need for update
    }

    std::string now_ch = (!this->chat_list.empty())? this->chat_list.now_choice_get()->chat_name : "";
    this->clear_chats();
    bool is_public;
    std::string chat_name;
    std::string user_name;
    std::string message;
    int state = 0; // 0 - is_public, 1 - chat_name, 2 - user_name, 3 - message
    for (int i = 0; i < chats.size(); ++i) {
        if (state == 0) {
            is_public = (chats[i] == CMD_ARG_GROUP);
            state++;
        }
        else if (state == 1) {
            chat_name = chats[i];
            state++;
        } else if (state == 2) {
            user_name = chats[i];
            state++;
        } else if (state == 3) {
            message = chats[i];
            this->chat_list.add(is_public, chat_name, TUI_COLOR_BLUE, user_name,
                                          TUI_COLOR_MAGENTA, message, TUI_COLOR_WHITE);
            if (chat_name == now_ch)
                this->chat_list.set_now_choice(i / 4);
            state = 0;
        }
    }
    if (now_ch.empty())
        this->chat_list.set_now_choice(0);
    this->print_chats();
}

void TUI::fix_cursor() {
    if (this->now_window == this->input_window) {
        int wx = getmaxx(this->input_window->overlay);
        int len_msg = this->input_text.size();
        wmove(this->input_window->overlay, len_msg/wx, len_msg%wx);
    } else {
        wmove(this->now_window->overlay, 0, 0);
    }
    wnoutrefresh(this->now_window->overlay);
}

void TUI::toggle_window() {
    if (this->now_window == this->input_window)
        this->now_window = this->chats_window;
    else
        this->now_window = this->input_window;
    fix_cursor();
}
