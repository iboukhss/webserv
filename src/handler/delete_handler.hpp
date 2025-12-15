#ifndef HANDLER_DELETE_HANDLER_HPP_
#define HANDLER_DELETE_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

#include <string>

class DeleteHandler : public Handler {
public:
    explicit DeleteHandler(const std::string& path);
    virtual ~DeleteHandler();

    virtual size_t read_data(char* buf, size_t n);
    virtual size_t write_data(const char* buf, size_t n);

    virtual bool has_output() const { return !headers_sent(); }
    virtual bool needs_input() const { return false; };
    virtual bool is_done() const { return !has_output(); }
    const std::string& path() const { return file_path_; }

    virtual int cgi_read_fd() const { return -1; };
    virtual int cgi_write_fd() const { return -1; };

private:
    DeleteHandler(const DeleteHandler&);
    DeleteHandler& operator=(const DeleteHandler&);

    bool headers_sent() const { return headers_off_ == headers_.size(); }

    const std::string file_path_;

    std::string headers_;
    size_t headers_off_;
};

#endif
