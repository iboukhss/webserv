#include "http/http_response.hpp"

#include <sstream>

const char* HttpResponse::reason_phrase(HttpResponse::Status code) const
{
    switch (code) {
    case kOk:                  return "OK";
    case kBadRequest:          return "Bad Request";
    case kForbidden:           return "Forbidden";
    case kNotFound:            return "Not Found";
    case kMethodNotAllowed:    return "Method Not Allowed";
    case kInternalServerError: return "Internal Server Error";
    case kNotImplemented:      return "Not Implemented";
    }
}

std::string HttpResponse::to_string() const
{
    std::ostringstream out;

    out << http_version << " " << code << " " << reason_phrase(code) << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        out << it->first << ": " << it->second << "\r\n";
    }
    out << "\r\n";
    out << body;

    return out.str();
}
