#include "handler/static_file_handler.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>

StaticFileHandler::StaticFileHandler(const std::string& path, const HttpRequest& saved_request)
    : file_path_(path),
      saved_request_(saved_request),
      fd_(-1),
      file_size_(0),
      eof_reached_(false),
      headers_off_(0)
{
    struct stat file_stat;
    HttpResponse res(saved_request_.http_version);

    if (stat(path.c_str(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        LOG(ERROR) << "Couldn't open file " << path;
        res.code = HttpResponse::kStatusNotFound;
        res.content_type = "text/html; charset=UTF-8"; // Magic to display emojis
        res.inline_body = "<h1>404 Not Found 😢</h1>\n";
        headers_ = res.to_string();
        return;
    }

    fd_ = open(path.c_str(), O_RDONLY);
    if (fd_ == -1) {
        res.code = HttpResponse::kStatusInternalServerError;
        headers_ = res.to_string();
        return;
    }

    file_size_ = file_stat.st_size;

    res.code = HttpResponse::kStatusOk;
    res.content_type = derive_file_type();
    res.content_length = file_size_;
    res.keep_alive = true;

    headers_ = res.to_string();
}

StaticFileHandler::~StaticFileHandler()
{
    if (fd_ != -1)
        close(fd_);
}

size_t StaticFileHandler::read_data(char* buf, size_t n)
{
    size_t bytes_written = 0;

    if (!headers_sent()) {
        size_t hdrs_bytes = headers_.size() - headers_off_;
        size_t to_copy = std::min(hdrs_bytes, n);

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
size_t StaticFileHandler::write_data(const char* buf, size_t n)
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
        return "text/html; charset=UTF-8";
    else if (ext == "txt")
        return "text/plain; charset=UTF-8";
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
