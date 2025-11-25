#include "util/string.hpp"

static void trim_left(std::string& str)
{
    std::string::iterator it = str.begin();

    while (it != str.end() && (*it == ' ' || *it == '\t')) {
        ++it;
    }
    str.erase(str.begin(), it);
}

static void trim_right(std::string& str)
{
    std::string::reverse_iterator rit = str.rbegin();

    while (rit != str.rend() && (*rit == ' ' || *rit == '\t')) {
        ++rit;
    }
    str.erase(rit.base(), str.end());
}

std::string str_trim(const std::string& str)
{
    std::string copy = str;

    trim_left(copy);
    trim_right(copy);
    return copy;
}
