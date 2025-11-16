#ifndef HTTP_HTTP_RESPONSE_HPP_
#define HTTP_HTTP_RESPONSE_HPP_

#include <map>
#include <string>

struct HttpResponse {
public:
    enum Status {
        kOk = 200,
        kBadRequest = 400,
        kForbidden = 403,
        kNotFound = 404,
        kMethodNotAllowed = 405,
        kInternalServerError = 500,
        kNotImplemented = 501
    };

    std::string http_version;
    Status code;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string to_string() const;

private:
    const char* reason_phrase(Status code) const;
};

#endif // HTTP_HTTP_RESPONSE_HPP_
