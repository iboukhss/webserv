#include "http/HttpResponse.hpp"

#include <sstream>

std::string HttpResponse::to_string() const
{
    std::ostringstream out;
    std::string reason;

    switch (status) {
    case 200:
        reason = "OK";
        break;
    case 404:
        reason = "Not Found";
        break;
    default:
        reason = "Unknown";
        break;
    }

    out << "HTTP/1.1 " << status << " " << reason << "\r\n";
    out << "Content-Type: " << content_type << "\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "\r\n";
    out << body;

    return out.str();
}
