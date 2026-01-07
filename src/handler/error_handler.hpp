#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "handler/handler.hpp"
#include "http/http_response.hpp"

#include <sys/stat.h>

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(HttpResponse::Status code);
    virtual ~ErrorHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const { return (!res_sent_); }
    virtual bool needs_input() const { return (false); }
    virtual bool is_done() const { return (res_sent_); }

    int error_code() { return res_.code; }

    virtual int cgi_read_fd() const { return -1; };
    virtual int cgi_write_fd() const { return -1; };

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    // NOTE: HttpResponse is default constructed with HTTP/1.1 + keep-alive
    HttpResponse res_;
    bool res_sent_;
};

#endif
