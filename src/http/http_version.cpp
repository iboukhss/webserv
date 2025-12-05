#include "http/http_version.hpp"

const char* http_version_to_string(HttpVersion protocol)
{
    switch (protocol) {
    case kHttpVersion1_0: return "HTTP/1.0";
    case kHttpVersion1_1: return "HTTP/1.1";
    default:              return "Unknown";
    }
}

HttpVersion http_version_from_string(const std::string& str)
{
    if (str == "HTTP/1.0")
        return kHttpVersion1_0;
    if (str == "HTTP/1.1")
        return kHttpVersion1_1;
    return kHttpVersionUnknown;
}
