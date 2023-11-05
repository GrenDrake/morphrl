#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "morph.h"


int percentOf(int percent, int ofValue) {
    return percent * 100 / ofValue;
}

std::string ucFirst(std::string text) {
    if (text.empty()) return text;
    if (text[0] >= 'a' && text[0] <= 'z') {
        text[0] -= 32;
    }
    return text;
}

std::string& trim(std::string &text) {
    if (text.empty()) return text;

    auto start = text.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        text.clear();
        return text;
    }
    text.erase(0, start);
    auto end = text.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) text.erase(end + 1);
    return text;
}

const std::string& trim(const std::string &text) {
    std::string copiedText(text);
    return trim(copiedText);
}

std::vector<std::string> explode(const std::string &text, char onChar) {
    std::vector<std::string> parts;
    std::string::size_type pos, lastpos = 0;

    pos = text.find_first_of(onChar);
    if (pos == std::string::npos) {
        parts.push_back(text);
        return parts;
    }

    while (pos != std::string::npos) {
        std::string part = text.substr(lastpos, pos - lastpos);
        parts.push_back(trim(part));
        lastpos = pos + 1;
        pos = text.find_first_of(onChar, lastpos);
    }
    std::string part = text.substr(lastpos);
    parts.push_back(trim(part));
    return parts;
}

std::vector<std::string> explodeOnWhitespace(std::string text) {
    const char *whitespaceChars = " \t\n\r";
    std::vector<std::string> parts;
    std::string::size_type pos, lastpos = 0;

    trim(text);
    if (text.empty()) return parts;

    pos = text.find_first_of(whitespaceChars);
    if (pos == std::string::npos) {
        parts.push_back(text);
        return parts;
    }

    while (pos != std::string::npos) {
        std::string part = text.substr(lastpos, pos - lastpos);
        trim(part);
        if (!part.empty()) parts.push_back(part);
        lastpos = pos;
        pos = text.find_first_of(whitespaceChars, lastpos);
        pos = text.find_first_not_of(whitespaceChars, pos);
    }
    std::string part = text.substr(lastpos);
    parts.push_back(trim(part));
    return parts;
}

bool strToInt(const std::string &text, int &result) {
    char *endPtr = nullptr;
    long num = strtol(text.c_str(), &endPtr, 10);
    if (*endPtr != 0) return false;
    result = num;
    return true;
}

template<typename CharT>
struct Sep : public std::numpunct<CharT> {
    virtual std::string do_grouping() const { return "\003"; }
};

std::string intToString(long long number) {
    std::stringstream line;
    line.imbue(std::locale(std::cout.getloc(), new Sep <char>()));
    line << number;
    return line.str();
}
