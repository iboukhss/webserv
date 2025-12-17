
#include "util/to_string.hpp"

std::string to_string(int n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}
