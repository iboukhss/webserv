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
    else if (ext == "svg")
        return "image/svg+xml";
    else if (ext == "gif")
        return "image/gif";
    else if (ext == "ico")
        return "image/x-icon";
    else
        return "application/octet-stream";
}

StaticFileHandler::StaticFileHandler(const std::string& path,
                                     const RouteConfig& rc,
                                     const HttpRequest& saved_request)
    : fd_(-1),
      file_size_(0),
      eof_reached_(false),
      headers_off_(0),
      rc_(rc)
{
    HttpResponse res(saved_request.http_version);

    if (rc.shared.index_files.empty())
        LOG(WARN) << "Index files are empty";

    std::string full_path = resolve_path(path, rc.shared.index_files);

    if (full_path.empty()) {
        LOG(ERROR) << "Couldn't open file " << path;
        /*res.code = HttpResponse::kStatusNotFound;
        res.content_type = "text/html; charset=UTF-8"; // Magic to display emojis
        res.inline_body = "<h1>404 Not Found 😢</h1>\n";*/
        headers_ = HttpResponse::make_error(HttpResponse::kStatusNotFound, rc_.shared.error_pages)
                       .to_string();
        return;
    }

    struct stat file_stat;

    fd_ = open(full_path.c_str(), O_RDONLY);
    stat(full_path.c_str(), &file_stat);
    file_size_ = file_stat.st_size;

    res.code = HttpResponse::kStatusOk;
    res.content_type = derive_file_type(full_path);
    res.content_length = file_size_;
    res.keep_alive = true;

    headers_ = res.to_string();
}

StaticFileHandler::~StaticFileHandler()
{
    if (fd_ != -1)
        close(fd_);
}

size_t StaticFileHandler::read_output(char* buf, size_t n)
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
            throw std::runtime_error("read failed");
        }

        if (body_bytes == 0) {
            eof_reached_ = true;
        }
        bytes_written += body_bytes;
    }
    return bytes_written;
}

// We never write to this handler (read-only)
size_t StaticFileHandler::write_input(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}
