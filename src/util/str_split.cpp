#include "util/string.hpp"

#include <stdexcept>

std::vector<std::string> str_split(const std::string& str, const std::string& delim)
{
    if (delim.empty())
        throw std::invalid_argument("str_split: empty delimiter");

    std::vector<std::string> tokens;
    size_t beg = 0;
    size_t end = 0;

    while ((end = str.find(delim, beg)) != std::string::npos) {
        tokens.push_back(str.substr(beg, end - beg));
        beg = end + delim.size();
    }

    tokens.push_back(str.substr(beg));
    return tokens;
}
