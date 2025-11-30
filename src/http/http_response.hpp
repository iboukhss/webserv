#ifndef HTTP_HTTP_RESPONSE_HPP_
#define HTTP_HTTP_RESPONSE_HPP_

#include <string>

struct HttpResponse {
public:
    enum Status {
        kStatusOk = 200,
        kStatusBadRequest = 400,
        kStatusForbidden = 403,
        kStatusNotFound = 404,
        kStatusMethodNotAllowed = 405,
        kStatusInternalServerError = 500,
        kStatusNotImplemented = 501
    };

    HttpResponse();

    std::string http_version;
    HttpResponse::Status code;

    // No need for generic headers here with a string map, we should already
    // know what kind of headers we support on the server.
    // It also makes sense to mirror what we have on the request side.
    // Caution when rearranging the order below, it will break tests.

    std::string content_type;
    size_t content_length;
    bool is_chunked;
    bool keep_alive;

    // Only use this to store small, well-known, predefined bodies.
    // Regular file data should be streamed via the handler interface.
    // I think this is bad design but it seems to be useful.
    std::string inline_body;

    std::string to_string() const;

private:
    const char* reason_phrase(HttpResponse::Status code) const;
};

#endif // HTTP_HTTP_RESPONSE_HPP_
