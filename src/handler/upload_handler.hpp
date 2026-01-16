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

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool is_regular_file() const { return true; }
    virtual bool has_output() const;
    virtual bool needs_input() const;
    virtual bool is_done() const;

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

private:
    UploadHandler(const UploadHandler&);
    UploadHandler& operator=(const UploadHandler&);

    void set_error(const HttpResponse::Status code);
    void finalize_upload();

    // constructor args
    const std::string& path_;
    const RouteConfig& rc_;
    const HttpRequest& req_;
    // Response built
    HttpResponse res_;
    // Serialized response and offset
    std::string out_buf_;
    // other handler specifc variables
    size_t total_bytes_written_;
    int fd_;
    bool is_upload_finished_;
    bool has_error_;
};

#endif
