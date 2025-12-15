#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "handler/handler.hpp"
#include "http/http_response.hpp"

#include <sys/stat.h>

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(HttpResponse::Status code);
    virtual ~ErrorHandler();

    virtual int get_read_fd() const { return read_fd_; };
    virtual int get_write_fd() const { return write_fd_; };

    virtual size_t read_data(char* buf, size_t n);
    virtual size_t write_data(const char* buf, size_t n);

    virtual bool has_output() const { return (!res_sent_); }
    virtual bool needs_input() const { return (false); }
    virtual bool is_done() const { return (res_sent_); }

    int error_code() { return res_.code; }

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    // NOTE: HttpResponse is default constructed with HTTP/1.1 + keep-alive
    HttpResponse res_;
    bool res_sent_;

    int read_fd_;
    int write_fd_;
};

#endif
