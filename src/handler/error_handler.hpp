#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(const HttpResponse::Status code, const RouteConfig& rc,
                          const HttpRequest& req_);
    virtual ~ErrorHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool is_regular_file() const { return true; }
    virtual bool has_output() const { return out_off_ < out_buf_.size(); }
    virtual bool needs_input() const { return false; }
    virtual bool is_done() const { return out_off_ >= out_buf_.size(); }

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

    HttpResponse::Status error_code() const { return res_.code; }

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    // constructor args
    const RouteConfig& rc_;
    const HttpRequest& req_;
    // Response built
    HttpResponse res_;
    // Serialized response and offset
    std::string out_buf_;
    size_t out_off_;
};

#endif
