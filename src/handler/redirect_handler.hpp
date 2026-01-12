#ifndef HANDLER_REDIRECT_HANDLER_HPP_
#define HANDLER_REDIRECT_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <string>

class RedirectHandler : public Handler {
public:
    explicit RedirectHandler(const RouteConfig& rc, const HttpRequest& req);
    virtual ~RedirectHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const { return out_off_ < out_buf_.size(); }
    virtual bool needs_input() const { return false; }
    virtual bool is_done() const { return out_off_ >= out_buf_.size(); }

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

private:
    RedirectHandler(const RedirectHandler&);
    RedirectHandler& operator=(const RedirectHandler&);
    void set_error(const HttpResponse::Status);

    // constructor args const
    const RouteConfig& rc_;
    const HttpRequest& req_;
    // Response built
    HttpResponse res_;
    // Serialized response and offset
    std::string out_buf_;
    size_t out_off_;
};
#endif
