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
#include <stdexcept>

UploadHandler::UploadHandler(const std::string& path, const RouteConfig& rc, const HttpRequest& req)
    : path_(path),
      rc_(rc),
      req_(req),
      total_bytes_written_(0),
      fd_(-1),
      is_upload_finished_(false),
      has_error_(false)
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

bool UploadHandler::has_output() const
{
    return !out_buf_.empty();
}

bool UploadHandler::needs_input() const
{
    return !has_error_ && !is_upload_finished_ && total_bytes_written_ < req_.content_length;
}

bool UploadHandler::is_done() const
{
    return !needs_input() && !has_output();
}

size_t UploadHandler::read_output(char* buf, size_t n)
{
    if (!has_output()) {
        LOG(WARN) << "Tried to read 0 bytes from UploadHandler";
        return 0;
    }

    size_t to_copy = std::min(out_buf_.size(), n);
    std::memcpy(buf, out_buf_.data(), to_copy);
    out_buf_.erase(0, to_copy);

    return to_copy;
}

size_t UploadHandler::write_input(const char* buf, size_t n)
{
    if (!needs_input()) {
        LOG(WARN) << "Tried to write 0 bytes to UploadHandler";
        return 0;
    }
    if (total_bytes_written_ + n > rc_.shared.max_body_size) {
        set_error(HttpResponse::kStatusContentTooLarge);
        return 0;
    }

    ssize_t written = write(fd_, buf, n);
    if (written == -1) {
        set_error(HttpResponse::kStatusInternalServerError);
        return 0;
    }
    if (written == 0) {
        return 0;
    }

    assert(static_cast<size_t>(written) == n && "Partial write on regular file??");

    total_bytes_written_ += written;

    if (total_bytes_written_ == req_.content_length) {
        finalize_upload();
    }
    return written;
}

void UploadHandler::set_error(const HttpResponse::Status code)
{
    res_ = HttpResponse::make_error(code, rc_.shared.error_pages, req_);
    out_buf_ = res_.to_string();
    has_error_ = true;
}

void UploadHandler::finalize_upload()
{
    res_ = HttpResponse::make_response_headers_only(HttpResponse::kStatusCreated, "", 0, req_);
    out_buf_ = res_.to_string();
    is_upload_finished_ = true;
}
