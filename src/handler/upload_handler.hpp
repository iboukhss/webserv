#ifndef HANDLER_UPLOAD_HANDLER_HPP_
#define HANDLER_UPLOAD_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>

#include <string>

class UploadHandler : public Handler {
public:
    explicit UploadHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& req);
    virtual ~UploadHandler();

    virtual size_t read_output(char* buf, size_t n); // we should not read from this handler
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool is_regular_file() const { return true; }
    virtual bool has_output() const { return out_off_ < out_buf_.size(); }
    virtual bool needs_input() const { return bytes_written_ < req_.content_length; }
    virtual bool is_done() const { return done_ && out_off_ >= out_buf_.size(); }

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

    const std::string& path() const { return path_; } // still required ?

private:
    UploadHandler(const UploadHandler&);
    UploadHandler& operator=(const UploadHandler&);
    void set_error(const HttpResponse::Status code);

    // constructor args
    const std::string& path_;
    const RouteConfig& rc_;
    const HttpRequest& req_;
    // Response built
    HttpResponse res_;
    // Serialized response and offset
    std::string out_buf_;
    size_t out_off_;
    // other handler specifc variables
    size_t bytes_written_;
    int fd_;
    bool done_;
};

#endif
