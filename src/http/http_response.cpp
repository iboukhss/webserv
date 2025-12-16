#include "http/http_response.hpp"

#include <cstdlib>
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

struct HttpStatusInfo {
    int code;
    const char* reason;
};

/* clang-format off */
static const HttpStatusInfo kStatusTable[] = {
    { 200, "OK" },
    { 201, "Created" },
    { 204, "No Content" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 409, "Conflict" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 507, "Disk Full" }
};
/* clang-format on */

const char* HttpResponse::reason_phrase(HttpResponse::Status status)
{
    int code = static_cast<int>(status);

    for (size_t i = 0; i < sizeof(kStatusTable) / sizeof(kStatusTable[0]); i++) {
        if (kStatusTable[i].code == code)
            return kStatusTable[i].reason;
    }
    return "Unknown";
}

HttpResponse::Status HttpResponse::parse_status(const std::string& s)
{
    int code = std::atoi(s.c_str());

    for (size_t i = 0; i < sizeof(kStatusTable) / sizeof(kStatusTable[0]); i++) {
        if (kStatusTable[i].code == code)
            return static_cast<HttpResponse::Status>(code);
    }
    return HttpResponse::kStatusNone;
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
