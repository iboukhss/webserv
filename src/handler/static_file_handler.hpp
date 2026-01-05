#ifndef HANDLER_GET_HANDLER_HPP_
#define HANDLER_GET_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>

#include <string>

class StaticFileHandler : public Handler {
public:
    StaticFileHandler(const std::string& path,
                      const RouteConfig& rc,
                      const HttpRequest& saved_request);

    virtual ~StaticFileHandler();

    virtual size_t read_data(char* buf, size_t n);
    virtual size_t write_data(const char* buf, size_t n);

    virtual bool has_output() const { return !headers_sent() || (has_body() && !body_sent()); }
    virtual bool needs_input() const { return false; };
    virtual bool is_done() const { return !has_output(); }

    virtual int cgi_read_fd() const { return -1; };
    virtual int cgi_write_fd() const { return -1; };

private:
    StaticFileHandler(const StaticFileHandler&);
    StaticFileHandler& operator=(const StaticFileHandler&);

    bool has_body() const { return fd_ != -1; }
    bool headers_sent() const { return headers_off_ == headers_.size(); }
    bool body_sent() const { return eof_reached_; }

    int fd_;
    off_t file_size_;
    bool eof_reached_;
    std::string headers_;
    size_t headers_off_;
    const RouteConfig& rc_;
};

#endif
