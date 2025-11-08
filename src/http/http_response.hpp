#ifndef HTTP_HTTP_RESPONSE_HPP_
#define HTTP_HTTP_RESPONSE_HPP_

#include <map>
#include <string>

enum HttpStatus {
    kOk = 200,
    kBadRequest = 400,
    kForbidden = 403,
    kNotFound = 404,
    kMethodNotAllowed = 405,
    kInternalServerError = 500,
    kNotImplemented = 501
};

struct HttpResponse {
    int status;
    std::map<std::string, std::string> headers;

    std::string content_type;
    std::string body;
    std::string to_string() const;
};

#endif // HTTP_HTTP_RESPONSE_HPP_
