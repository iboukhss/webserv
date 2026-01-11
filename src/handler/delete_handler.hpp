#ifndef HANDLER_DELETE_HANDLER_HPP_
#define HANDLER_DELETE_HANDLER_HPP_

#include "config/server_config.hpp"
#include "handler/handler.hpp"

#include <sys/stat.h>

#include <string>

class DeleteHandler : public Handler {
public:
    explicit DeleteHandler(const std::string& path, const RouteConfig& rc);
    virtual ~DeleteHandler();

    virtual size_t read_output(char* buf, size_t n);
    virtual size_t write_input(const char* buf, size_t n);

    virtual bool has_output() const { return out_off_ < out_buf_.size(); }
    virtual bool needs_input() const { return false; }
    virtual bool is_done() const { return out_off_ >= out_buf_.size(); }

    const std::string& path() const { return file_path_; }
    void set_error(const HttpResponse::Status, const RouteConfig& rc);

    virtual int cgi_read_fd() const { return -1; }
    virtual int cgi_write_fd() const { return -1; }

private:
    DeleteHandler(const DeleteHandler&);
    DeleteHandler& operator=(const DeleteHandler&);

    const std::string file_path_;

    HttpResponse res_;

    std::string out_buf_;
    size_t out_off_;
};

#endif
