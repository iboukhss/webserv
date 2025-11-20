#include "handler/static_file_handler.hpp"

#include "http/http_response.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

StaticFileHandler::StaticFileHandler(const std::string& path)
    : file_path_(path),
      fd_(-1),
      file_size_(0),
      eof_reached_(false),
      headers_off_(0)
{
    struct stat file_stat;
    HttpResponse res;
    res.http_version = "HTTP/1.1";

    std::cout << "[DEBUG] Trying to open file: " << path << std::endl;

    if (stat(path.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        res.code = HttpResponse::kNotFound;
        headers_ = res.to_string();
        return;
    }

    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ == -1) {
        res.code = HttpResponse::kInternalServerError;
        headers_ = res.to_string();
        return;
    }

    file_size_ = file_stat.st_size;

    std::ostringstream oss;

    oss << file_size_;

    res.code = HttpResponse::kOk;
    res.headers["Content-Type"] = derive_file_type();
    res.headers["Content-Length"] = oss.str();
    res.headers["Connection"] = "keep-alive";

    headers_ = res.to_string();
}

StaticFileHandler::~StaticFileHandler()
{
    if (fd_ != -1)
        close(fd_);
}

int StaticFileHandler::read_data(char* buf, int n)
{
    int bytes_written = 0;

    if (!headers_sent()) {
        int hdrs_bytes = headers_.size() - headers_off_;
        int to_copy = std::min(hdrs_bytes, n);

        std::memcpy(buf, headers_.data() + headers_off_, to_copy);
        headers_off_ += to_copy;
        bytes_written += to_copy;

        if (bytes_written == n) {
            return bytes_written; // buffer full, cannot continue
        }
    }
    if (has_body() && !body_sent()) {
        int body_bytes = read(fd_, buf + bytes_written, n - bytes_written);
        if (body_bytes == -1) {
            throw UnrecoverableError("Failed to read file on disk", errno);
        }

        if (body_bytes == 0) {
            eof_reached_ = true;
        }
        bytes_written += body_bytes;
    }
    return bytes_written;
}

// We never write to this handler (read-only)
int StaticFileHandler::write_data(const char* buf, int n)
{
    (void) buf;
    (void) n;
    return 0;
}

const std::string StaticFileHandler::derive_file_type()
{
    size_t pos = file_path_.rfind(".");
    if (pos == std::string::npos)
        return ("application/octet-stream");

    std::string ext = file_path_.substr(pos + 1);
    if (ext == "html" || ext == "htm")
        return "text/html";
    else if (ext == "txt")
        return "text/plain";
    else if (ext == "css")
        return "text/css";
    else if (ext == "js")
        return "application/javascript";
    else if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    else if (ext == "png")
        return "image/png";
    else if (ext == "gif")
        return "image/gif";
    else if (ext == "ico")
        return "image/x-icon";
    else
        return "application/octet-stream";
}
