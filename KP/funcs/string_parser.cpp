#include "string_parser.h"

void string_parser(std::string st, int args_count, std::vector<std::string>& to_parse_vec, char separator) {
    int prev_space_ind = -1;
    int i = 0;
    args_count--;
    while (args_count > 0 && i < st.size()) {
        if (st[i] == separator) {
            args_count--;
            to_parse_vec.push_back(st.substr(prev_space_ind+1, i-prev_space_ind-1));
            prev_space_ind = i;
        }
        i++;
    }
    if (prev_space_ind < int(st.size()-1))
        to_parse_vec.push_back(st.substr(prev_space_ind+1, st.size()-prev_space_ind-1));
    else
        to_parse_vec.emplace_back("");
}