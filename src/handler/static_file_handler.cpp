#include "handler/static_file_handler.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <vector>

static std::string resolve_path(const std::string& path,
                                const std::vector<std::string>& index_files)
{
    struct stat sb;
    std::string full_path;
    std::string not_found = "";

    // Happy case: exact match found
    if (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
        return path;

    // Directory: check every index files
    if (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        for (size_t i = 0; i < index_files.size(); i++) {
            full_path = path + "/" + index_files[i];
            if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
                return full_path;
        }
        return not_found;
    }

    // Clean URL fallback: check /foo -> /foo.html
    full_path = path + ".html";
    if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode))
        return full_path;

    return not_found;
}

static const char* derive_file_type(const std::string& file_path)
{
    size_t pos = file_path.rfind(".");
    if (pos == std::string::npos)
        return ("application/octet-stream");

    std::string ext = file_path.substr(pos + 1);
    if (ext == "html" || ext == "htm")
        return "text/html; charset=UTF-8";
    if (ext == "txt")
        return "text/plain; charset=UTF-8";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "png")
        return "image/png";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "gif")
        return "image/gif";
    if (ext == "ico")
        return "image/x-icon";
    return "application/octet-stream";
}

void StaticFileHandler::set_error(const HttpResponse::Status code, const RouteConfig& rc)
{
    res_ = HttpResponse::make_error(code, rc.shared.error_pages);
    out_buf_ = res_.to_string();
}

StaticFileHandler::StaticFileHandler(const std::string& path, const RouteConfig& rc)
    : fd_(-1),
      rc_(rc),
      file_size_(0),
      out_off_(0)
{

    if (rc.shared.index_files.empty())
        LOG(WARN) << "Index files are empty";
    std::string full_path = resolve_path(path, rc.shared.index_files);
    if (full_path.empty()) {
        LOG(ERROR) << "Couldn't open file " << path;
        set_error(HttpResponse::kStatusNotFound, rc_);
        return;
    }
    struct stat file_stat;
    fd_ = open(full_path.data(), O_RDONLY);
    if (fd_ == -1) {
        set_error(HttpResponse::kStatusInternalServerError, rc_);
        return;
    }
    stat(full_path.data(), &file_stat);
    file_size_ = file_stat.st_size;
    std::string file_type = derive_file_type(full_path);
    res_ = HttpResponse::make_response_headers_only(HttpResponse::kStatusOk, file_type, file_size_);
    out_buf_ = res_.to_string();
}

StaticFileHandler::~StaticFileHandler()
{
    if (fd_ != -1)
        close(fd_);
}

size_t StaticFileHandler::read_output(char* buf, size_t n)
{
    size_t copied = 0;
    // first copy what remainder in out_buf_
    if (out_off_ < out_buf_.size()) {
        size_t bytes_left = out_buf_.size() - out_off_;
        copied = std::min(bytes_left, n);
        std::memcpy(buf, out_buf_.data() + out_off_, copied);
        out_off_ += copied;
        if (copied == n) {
            return (copied); // buffer full
        }
    }
    if (fd_ == -1) // safeguard
        return copied;
    ssize_t bytes = read(fd_, buf + copied, n - copied);
    if (bytes == 0) {
        close(fd_);
        fd_ = -1;
        return copied;
    }
    if (bytes < 0) {
        throw std::runtime_error("read failed");
    }
    copied += bytes;
    return (copied);
}

// We never write to this handler (read-only)
size_t StaticFileHandler::write_input(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}
