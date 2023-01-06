#include "text_buffer.h"

TextBufElem* TextBuffer::add(const std::string &text, int color) {
    auto el = new TextBufElem;
    el->text = text;
    el->color = color;
    this->buffer.push_back(el);
    return el;
}

void TextBuffer::clear() {
    for (auto &el: this->buffer) {
        delete el;
    }
    this->buffer.clear();
}

TextBuffer::~TextBuffer() {
    clear();
}

std::vector<TextBufElem *>::iterator TextBuffer::begin() {
    return this->buffer.begin();
}

std::vector<TextBufElem *>::iterator TextBuffer::end() {
    return this->buffer.end();
}
