#include "http/http_response.hpp"

#include <sstream>

std::string HttpResponse::to_string() const
{
    std::ostringstream out;
    std::string reason;

    /* clang-format off */
    switch (status) {
    case kOk: reason = "OK"; break;
    case kBadRequest: reason = "Bad Request"; break;
    case kForbidden: reason = "Forbidden"; break;
    case kNotFound: reason = "Not Found"; break;
    case kMethodNotAllowed: reason = "Method not allowed"; break;
    case kInternalServerError: reason = "Internal Server Error"; break;
    case kNotImplemented: reason = "Not Implemented"; break;
    default: reason = "Unknown"; break;
    }
    /* clang-format on */

    out << "HTTP/1.1 " << status << " " << reason << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        out << it->first << ": " << it->second << "\r\n";
    }
    if (headers.find("Content-Length") == headers.end())
        out << "Content-Length: " << body.size() << "\r\n";
    out << "\r\n";
    out << body;

    return out.str();
}
