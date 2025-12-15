#ifndef HANDLER_DELETE_HANDLER_HPP_
#define HANDLER_DELETE_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

#include <string>

class DeleteHandler : public Handler {
public:
    explicit DeleteHandler(const std::string& path);
    virtual ~DeleteHandler();

    virtual int get_read_fd() const { return read_fd_; };
    virtual int get_write_fd() const { return write_fd_; };

    virtual size_t read_data(char* buf, size_t n);
    virtual size_t write_data(const char* buf, size_t n);

    virtual bool has_output() const { return !headers_sent(); }
    virtual bool needs_input() const { return false; };
    virtual bool is_done() const { return !has_output(); }
    const std::string& path() const { return file_path_; }

private:
    DeleteHandler(const DeleteHandler&);
    DeleteHandler& operator=(const DeleteHandler&);

    bool headers_sent() const { return headers_off_ == headers_.size(); }

    const std::string file_path_;

    std::string headers_;
    size_t headers_off_;

    int read_fd_;
    int write_fd_;
};

#endif
