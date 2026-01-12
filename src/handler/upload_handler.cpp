#include "handler/upload_handler.hpp"

#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

UploadHandler::UploadHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& req)
    : path_(path),
      rc_(rc),
      req_(req),
      out_off_(0),
      bytes_written_(0),
      fd_(-1)

{
    if (req.content_length == 0) {
        // set_error(HttpResponse::kStatusBadRequest);
        // set_error(HttpResponse::kStatusNoContent);
        set_error(HttpResponse::kStatusOk);
        return;
    }
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
        set_error(HttpResponse::kStatusConflict); // file already exists
        return;
    }
    fd_ = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_ == -1) {
        set_error(HttpResponse::kStatusConflict); // file could not be truncated
        return;
    }
}

UploadHandler::~UploadHandler()
{
    if (fd_ != -1)
        close(fd_);
}

size_t UploadHandler::read_output(char* buf, size_t n)
{
    if (out_off_ >= out_buf_.size()) {
        return 0;
    }
    size_t bytes_left = out_buf_.size() - out_off_;
    size_t to_copy = std::min(bytes_left, n);
    std::memcpy(buf, out_buf_.data() + out_off_, to_copy);
    out_off_ += to_copy;
    return (to_copy);
}

size_t UploadHandler::write_input(const char* buf, size_t n)
{
    ssize_t bytes = write(fd_, buf, n);
    if (bytes < 0) {
        if (out_buf_.empty()) {
            set_error(HttpResponse::kStatusDiskFull);
        }
        bytes_written_ = req_.content_length; // to ensure needs_input returns false
        return 0;
    }
    bytes_written_ += bytes;
    if (bytes_written_ < req_.content_length) {
        return (bytes);
    }
    if (out_buf_.empty()) {
        res_ = HttpResponse::make_response_headers_only(HttpResponse::kStatusCreated, "", 0, req_);
        out_buf_ = res_.to_string();
    }
    return (0);
}

void UploadHandler::set_error(const HttpResponse::Status code)
{
    res_ = HttpResponse::make_error(code, rc_.shared.error_pages, req_);
    out_buf_ = res_.to_string();
}
