#ifndef KP_TEXT_BUFFER_H
#define KP_TEXT_BUFFER_H
#include <vector>
#include <string>

struct TextBufElem {
    std::string text;
    int color;
};

class TextBuffer {
public:
    TextBufElem* add(const std::string& text, int color);
    std::vector<TextBufElem*>::iterator begin();
    std::vector<TextBufElem*>::iterator end();
    void clear();
    ~TextBuffer();
private:
    std::vector<TextBufElem*> buffer;
};

#endif //KP_TEXT_BUFFER_H
