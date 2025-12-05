#include "http/http_response.hpp"

#include <sstream>

// TODO(isma): Maybe make a proper constructor for this class?
// TODO(isma): Add more assertions to ensure the logic holds.

HttpResponse::HttpResponse(HttpVersion protocol)
    : http_version(protocol),
      code(HttpResponse::kStatusOk),
      is_chunked(false),
      keep_alive(http_version == kHttpVersion1_1)
{
}

const char* HttpResponse::reason_phrase(HttpResponse::Status code) const
{
    switch (code) {
    case kStatusOk:                  return "OK";
    case kStatusCreated:             return "File Created";
    case kStatusNoContent:           return "No Content";
    case kStatusBadRequest:          return "Bad Request";
    case kStatusForbidden:           return "Forbidden";
    case kStatusNotFound:            return "Not Found";
    case kStatusMethodNotAllowed:    return "Method Not Allowed";
    case kStatusConflict:            return "Conflict";
    case kStatusInternalServerError: return "Internal Server Error";
    case kStatusNotImplemented:      return "Not Implemented";
    case kStatusDiskFull:            return "Disk Full";
    }
}

std::string HttpResponse::to_string() const
{
    std::ostringstream out;

    out << http_version_to_string(http_version) << " " << code << " " << reason_phrase(code)
        << "\r\n";

    if (!content_type.empty()) {
        out << "Content-Type: " << content_type << "\r\n";
    }
    if (!inline_body.empty()) {
        out << "Content-Length: " << inline_body.size() << "\r\n";
    }
    else if (content_length > 0) {
        out << "Content-Length: " << content_length << "\r\n";
    }
    if (is_chunked) {
        out << "Transfer-Encoding: chunked\r\n";
    }

    out << "Connection: " << (keep_alive ? "keep-alive" : "close") << "\r\n";

    out << "\r\n";
    out << inline_body;

    return out.str();
}
