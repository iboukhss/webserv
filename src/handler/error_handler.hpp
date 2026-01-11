#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_response.hpp"

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(const HttpResponse::Status code, const RouteConfig& rc);
    virtual ~ErrorHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const { return out_off_ < out_buf_.size(); }
    virtual bool needs_input() const { return false; }
    virtual bool is_done() const { return out_off_ >= out_buf_.size(); }

    HttpResponse::Status error_code() const { return res_.code; }

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    HttpResponse res_;

    std::string out_buf_;
    size_t out_off_;
};

#endif
