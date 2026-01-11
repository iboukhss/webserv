#ifndef HANDLER_UPLOAD_HANDLER_HPP_
#define HANDLER_UPLOAD_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"

#include <sys/stat.h>

#include <string>

class UploadHandler : public Handler {
public:
    explicit UploadHandler(const std::string& path, const RouteConfig& rc, size_t content_lenght);
    virtual ~UploadHandler();

    virtual size_t read_output(char* buf, size_t n); // we should not read from this handler
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const { return out_off_ < out_buf_.size(); }
    virtual bool needs_input() const { return bytes_written_ < content_length_; }
    virtual bool is_done() const { return out_off_ >= out_buf_.size(); }

    void set_error(const HttpResponse::Status code, const RouteConfig& rc);
    const std::string& path() const { return file_path_; }

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

private:
    UploadHandler(const UploadHandler&);
    UploadHandler& operator=(const UploadHandler&);

    const std::string file_path_;
    int fd_;
    const RouteConfig& rc_;

    size_t bytes_written_;
    size_t content_length_;

    HttpResponse res_;
    std::string out_buf_;
    size_t out_off_;
};

#endif
