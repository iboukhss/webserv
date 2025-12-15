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

/*
ERRORS :
insufficient space → 507 Insufficient Storage

forbidden directory → 403

invalid request format → 400

If file exists → POST = 409 Conflict


Also, the UploadHandler could handle, both POST and PUT requests

*/

UploadHandler::UploadHandler(const std::string& path, size_t content_length)
    : file_path_(path),
      bytes_written_(0),
      content_length_(content_length),
      eob_reached_(false),
      headers_off_(0),
      read_fd_(-1),
      write_fd_(-1)
{
    struct stat file_stat;
    if (stat(path.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
        // file already exists
        HttpResponse res;
        res.code = HttpResponse::kStatusConflict; // 409
        res.inline_body = "<h1> 409 Conflict — File already exists </h1>";
        headers_ = res.to_string();
        return;
    }
    write_fd_ = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (write_fd_ == -1) {
        HttpResponse res;
        res.code = HttpResponse::kStatusDiskFull;
        res.inline_body = "<h1> 507 Disk Full </h1>"; // to display a message during testing
        headers_ = res.to_string();
        return;
    }
}

UploadHandler::~UploadHandler()
{
    if (read_fd_ != -1) {
        close(read_fd_);
        read_fd_ = -1;
    }
    if (write_fd_ != -1) {
        close(write_fd_);
        write_fd_ = -1;
    }
}

bool UploadHandler::needs_input() const
{
    return bytes_written_ < content_length_;
}

bool UploadHandler::has_output() const
{
    return headers_off_ < headers_.size();
}

size_t UploadHandler::read_data(char* buf, size_t n)
{
    // LOG(DEBUG) << "inside UploadHandler::read_data -> headers_.size == " << headers_.size();
    size_t to_copy = std::min(headers_.size() - headers_off_, n);
    std::memcpy(buf, headers_.c_str() + headers_off_, to_copy);
    headers_off_ += to_copy;
    return (to_copy);
}

size_t UploadHandler::write_data(const char* buf, size_t n)
{
    ssize_t bytes = write(write_fd_, buf, n);
    if (bytes == -1) {
        // error occured
        eob_reached_ = true;
        if (headers_.empty()) {
            HttpResponse res;
            res.code = HttpResponse::kStatusDiskFull;
            res.inline_body = "<h1> 507 Disk Full </h1>"; // to display a message during testing
            headers_ = res.to_string();
            bytes_written_ = content_length_;             // to ensure needs_input returns false
        }
        return 0;
    }
    bytes_written_ += bytes;
    if (bytes_written_ < content_length_) {

        return (bytes);
    }

    if (headers_.empty()) {
        eob_reached_ = true;
        HttpResponse res;
        res.code = HttpResponse::kStatusCreated;
        res.inline_body = "<h1> 201 File Created </h1>"; // to display a message during testing
        headers_ = res.to_string();
    }
    return (0);
}
