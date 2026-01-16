#ifndef HTTP_HTTP_RESPONSE_HPP_
#define HTTP_HTTP_RESPONSE_HPP_

#include "core/server_defaults.hpp"
#include "http/http_request.hpp"
#include "http/http_version.hpp"

#include <map>
#include <string>

struct HttpResponse {
public:
    // NOTE: Don't forget to update kStatusTable after adding new status codes
    // below.
    enum Status {
        kStatusNone = 0, // Sentinel value for initialization, never sent
        kStatusOk = 200,
        kStatusCreated = 201,
        kStatusNoContent = 204,
        kStatusMovedPermanently = 301,
        kStatusFound = 302,
        kStatusBadRequest = 400,
        kStatusForbidden = 403,
        kStatusNotFound = 404,
        kStatusMethodNotAllowed = 405,
        kStatusConflict = 409,
        kStatusContentTooLarge = 413,
        kStatusUriTooLong = 431,
        kStatusRequestHeaderFieldsTooLarge = 431,
        kStatusInternalServerError = 500,
        kStatusNotImplemented = 501,
        kStatusBadGateway = 502,
        kStatusDiskFull = 507
    };

    explicit HttpResponse(HttpVersion protocol = WEBSERV_DEFAULT_HTTP_VERSION);

    HttpVersion http_version;
    HttpResponse::Status code;

    // No need for generic headers here with a string map, we should already
    // know what kind of headers we support on the server.
    // It also makes sense to mirror what we have on the request side.
    // Caution when rearranging the order below, it will break tests.

    std::string content_type;
    size_t content_length;
    std::string location;
    bool is_chunked;
    bool keep_alive;

    // Only use this to store small, well-known, predefined bodies.
    // Regular file data should be streamed via the handler interface.
    // I think this is bad design but it seems to be useful.
    std::string inline_body;

    static const char* reason_phrase(HttpResponse::Status status);
    static HttpResponse::Status parse_status(const std::string& s);
    static Status status_from_int(int code);
    static HttpResponse make_error(HttpResponse::Status status,
                                   const std::map<HttpResponse::Status, std::string>& error_pages,
                                   const HttpRequest& req);
    static HttpResponse make_response_headers_only(HttpResponse::Status status,
                                                   const std::string& content_type,
                                                   size_t content_length, const HttpRequest& req);
    static HttpResponse make_response_with_body(HttpResponse::Status status,
                                                const std::string& content_type,
                                                const std::string& body, const HttpRequest& req);
    std::string to_string() const;
};

#endif // HTTP_HTTP_RESPONSE_HPP_
