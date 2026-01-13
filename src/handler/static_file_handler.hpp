#ifndef HANDLER_GET_HANDLER_HPP_
#define HANDLER_GET_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"
#include "http/http_request.hpp"

#include <sys/stat.h>

#include <string>

enum ResolveKind { kFile, kDirNoIndex, kNotFound };

struct ResolveResult {
    ResolveKind kind;
    std::string path;
};

class StaticFileHandler : public Handler {
public:
    StaticFileHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& req);

    virtual ~StaticFileHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool is_regular_file() const { return true; }
    virtual bool has_output() const { return out_off_ < out_buf_.size() || fd_ != -1; }
    virtual bool needs_input() const { return false; };
    virtual bool is_done() const { return out_off_ >= out_buf_.size() && fd_ == -1; }

    virtual int cgi_read_fd() const { return -1; };
    virtual int cgi_write_fd() const { return -1; };

private:
    StaticFileHandler(const StaticFileHandler&);
    StaticFileHandler& operator=(const StaticFileHandler&);
    void resolve_path(ResolveResult& resolve) const;
    void set_error(const HttpResponse::Status code);
    void set_redirect(const HttpResponse::Status code, const std::string& redirect_path);

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
    off_t file_size_;
    int fd_;
};

#endif
