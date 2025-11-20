#ifndef HANDLER_GET_HANDLER_HPP_
#define HANDLER_GET_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

#include <string>

class StaticFileHandler : public Handler {
public:
    explicit StaticFileHandler(const std::string& path);
    virtual ~StaticFileHandler();

    virtual int read_data(char* buf, int n);
    virtual int write_data(const char* buf, int n);

    virtual bool has_output() const { return !headers_sent() || (has_body() && !body_sent()); }
    virtual bool needs_input() const { return false; };
    virtual bool is_done() const { return !has_output(); }

private:
    StaticFileHandler(const StaticFileHandler&);
    StaticFileHandler& operator=(const StaticFileHandler&);

    bool has_body() const { return fd_ != -1; }
    bool headers_sent() const { return headers_off_ == headers_.size(); }
    bool body_sent() const { return eof_reached_; }

    const std::string derive_file_type();

    const std::string file_path_;

    int fd_;
    off_t file_size_;
    bool eof_reached_;
    std::string headers_;
    size_t headers_off_;
};

#endif
