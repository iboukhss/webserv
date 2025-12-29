#include "util/string.hpp"

#include <sstream>
#include <string>

std::string itoa(int val)
{
    std::ostringstream oss;

    oss << val;
    return oss.str();
}
