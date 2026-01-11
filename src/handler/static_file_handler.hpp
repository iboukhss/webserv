#ifndef HANDLER_GET_HANDLER_HPP_
#define HANDLER_GET_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>

#include <string>

class StaticFileHandler : public Handler {
public:
    StaticFileHandler(const std::string& path, const RouteConfig& rc);

    virtual ~StaticFileHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const { return out_off_ < out_buf_.size() || fd_ != -1; }
    virtual bool needs_input() const { return false; };
    virtual bool is_done() const { return out_off_ >= out_buf_.size() && fd_ == -1; }

    void set_error(const HttpResponse::Status code, const RouteConfig& rc);

    virtual int cgi_read_fd() const { return -1; };
    virtual int cgi_write_fd() const { return -1; };

private:
    StaticFileHandler(const StaticFileHandler&);
    StaticFileHandler& operator=(const StaticFileHandler&);

    int fd_;
    const RouteConfig& rc_;
    off_t file_size_;

    HttpResponse res_;

    std::string out_buf_;
    size_t out_off_;
};

#endif
