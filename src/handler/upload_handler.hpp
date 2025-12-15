#ifndef HANDLER_UPLOAD_HANDLER_HPP_
#define HANDLER_UPLOAD_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

#include <string>

class UploadHandler : public Handler {
public:
    explicit UploadHandler(const std::string& path, size_t content_lenght);
    virtual ~UploadHandler();

    virtual int get_read_fd() const { return read_fd_; };
    virtual int get_write_fd() const { return write_fd_; };

    virtual size_t read_data(char* buf, size_t n); // we should not read from this handler
    virtual size_t write_data(const char* buf, size_t n);

    virtual bool has_output() const;
    virtual bool needs_input() const;
    virtual bool is_done() const { return headers_sent(); }
    const std::string& path() const { return file_path_; }

private:
    UploadHandler(const UploadHandler&);
    UploadHandler& operator=(const UploadHandler&);

    bool headers_sent() const { return headers_off_ == headers_.size(); }
    bool body_written() const { return eob_reached_; }

    const std::string file_path_;

    size_t bytes_written_;
    size_t content_length_;
    bool eob_reached_;
    std::string headers_;
    size_t headers_off_;

    int read_fd_;
    int write_fd_;
};

#endif
