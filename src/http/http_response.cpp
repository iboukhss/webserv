#include "http/http_response.hpp"

#include "fcntl.h"
#include "sys/stat.h"
#include "unistd.h"

#include <cstdlib>
#include <map>
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
    { 502, "Bad Gateway"},
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

HttpResponse::Status HttpResponse::status_from_int(int code)
{
    switch (code) {
    case 200: return HttpResponse::kStatusOk;
    case 201: return HttpResponse::kStatusCreated;
    case 204: return HttpResponse::kStatusNoContent;
    case 301: return HttpResponse::kStatusMovedPermanently;
    case 302: return HttpResponse::kStatusFound;
    case 400: return HttpResponse::kStatusBadRequest;
    case 403: return HttpResponse::kStatusForbidden;
    case 404: return HttpResponse::kStatusNotFound;
    case 405: return HttpResponse::kStatusMethodNotAllowed;
    case 409: return HttpResponse::kStatusConflict;
    case 500: return HttpResponse::kStatusInternalServerError;
    case 501: return HttpResponse::kStatusNotImplemented;
    case 502: return HttpResponse::kStatusBadGateway;
    case 507: return HttpResponse::kStatusDiskFull;
    default:  return HttpResponse::kStatusInternalServerError;
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
    if (!location.empty()) {
        out << "Location: " << location << "\r\n";
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

HttpResponse
HttpResponse::make_error(HttpResponse::Status status,
                         const std::map<HttpResponse::Status, std::string>& error_pages,
                         const HttpRequest& req)
{
    HttpResponse res(req.http_version);
    res.code = status;
    res.content_type = "text/html; charset=UTF-8";
    res.keep_alive = req.keep_alive;
    std::map<HttpResponse::Status, std::string>::const_iterator it = error_pages.find(status);
    if (it != error_pages.end()) {
        res.inline_body = it->second;
        res.content_length = res.inline_body.size();
        return res;
    }
    int code = static_cast<int>(status);
    std::ostringstream oss; // send a minimal default html body containing the error status code
    oss << "<!doctype html><html><head><meta charset=\"utf-8\">"
           "<title>"
        << code << " " << HttpResponse::reason_phrase(status) << "</title></head><body><h1>" << code
        << " " << HttpResponse::reason_phrase(status) << "</h1></body></html>";
    res.inline_body = oss.str();
    res.content_length = res.inline_body.size();
    return res;
}

HttpResponse HttpResponse::make_response_headers_only(HttpResponse::Status status,
                                                      const std::string& content_type,
                                                      size_t content_length, const HttpRequest& req)
{
    HttpResponse res(req.http_version);
    res.code = status;
    if (!content_type.empty())
        res.content_type = content_type;
    res.keep_alive = req.keep_alive;
    res.content_length = content_length; // should be empty of content-length = 0
    return res;
}

HttpResponse HttpResponse::make_response_with_body(HttpResponse::Status status,
                                                   const std::string& content_type,
                                                   const std::string& body, const HttpRequest& req)
{
    HttpResponse res(req.http_version);
    res.code = status;
    res.content_type = content_type;
    res.keep_alive = req.keep_alive;
    if (status == 204 || status == 304) {
        res.inline_body = "";
        res.content_length = 0;
    }
    res.inline_body = body;
    res.content_length = res.inline_body.size();
    return res;
}
