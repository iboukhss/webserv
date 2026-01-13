#include "handler/static_file_handler.hpp"

#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <vector>

static std::vector<std::string> list_dir(const std::string& dir_path)
{
    std::vector<std::string> names;

    DIR* d = opendir(dir_path.c_str());
    if (!d)
        return names;

    for (dirent* ent = readdir(d); ent != NULL; ent = readdir(d)) {
        std::string name = ent->d_name;

        if (name == "." || name == "..")
            continue;

        names.push_back(name);
    }
    closedir(d);

    std::sort(names.begin(), names.end());
    return names;
}

static std::string html_escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '&')
            out += "&amp;";
        else if (c == '<')
            out += "&lt;";
        else if (c == '>')
            out += "&gt;";
        else if (c == '"')
            out += "&quot;";
        else
            out += c;
    }
    return out;
}

static std::string url_escape_min(const std::string& s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ' ')
            out += "%20";
        else
            out += s[i];
    }
    return out;
}

static std::string build_autoindex_html(const std::string& fs_dir, const std::string& url_dir)
{
    std::vector<std::string> names = list_dir(fs_dir);

    std::string body;
    body += "<!doctype html><html><head><meta charset=\"utf-8\">";
    body += "<title>Index of " + html_escape(url_dir) + "</title></head><body>";
    body += "<h1>Index of " + html_escape(url_dir) + "</h1>";
    body += "<ul>";

    // Parent link (optional)
    if (url_dir != "/") {
        body += "<li><a href=\"../\">../</a></li>";
    }

    for (size_t i = 0; i < names.size(); ++i) {
        const std::string& name = names[i];

        std::string fs_entry = fs_dir;
        if (!fs_entry.empty() && fs_entry[fs_entry.size() - 1] != '/')
            fs_entry += "/";
        fs_entry += name;

        struct stat sb;
        bool is_dir = (stat(fs_entry.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));

        std::string display = html_escape(name) + (is_dir ? "/" : "");
        std::string href = url_escape_min(name) + (is_dir ? "/" : "");

        body += "<li><a href=\"" + href + "\">" + display + "</a></li>";
    }

    body += "</ul></body></html>";
    return body;
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

void StaticFileHandler::resolve_path(ResolveResult& resolve) const
{
    struct stat sb;
    std::string full_path;
    const std::vector<std::string>& index_files = rc_.shared.index_files;
    // Happy case: exact match found
    if (stat(path_.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
        resolve.path = path_;
        resolve.kind = kFile;
        return;
    }
    // Directory: check every index files
    if (stat(path_.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        for (size_t i = 0; i < index_files.size(); i++) {
            full_path = path_ + "/" + index_files[i];
            if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
                resolve.path = full_path;
                resolve.kind = kFile;
                return;
            }
        }
        resolve.path = path_;
        resolve.kind = kDirNoIndex;
        return;
    }

    // Clean URL fallback: check /foo -> /foo.html
    full_path = path_ + ".html";
    if (stat(full_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
        resolve.path = full_path;
        resolve.kind = kFile;
        return;
    }

    resolve.path = "";
    resolve.kind = kNotFound;
    return;
}

StaticFileHandler::StaticFileHandler(const std::string& path,
                                     const RouteConfig& rc,
                                     const HttpRequest& req)
    : path_(path),
      rc_(rc),
      req_(req),
      out_off_(0),
      file_size_(0),
      fd_(-1)

{
    LOG(DEBUG) << "req_.path = " << req_.path;
    struct ResolveResult r;
    resolve_path(r);
    if (r.kind == kNotFound) {
        set_error(HttpResponse::kStatusNotFound);
        return;
    }
    if (r.kind == kDirNoIndex && !rc_.shared.autoindex_enabled) {
        set_error(HttpResponse::kStatusForbidden);
        return;
    }
    if (r.kind == kDirNoIndex && rc_.shared.autoindex_enabled && r.path[r.path.size() - 1] != '/') {
        set_redirect(HttpResponse::kStatusMovedPermanently, req_.path + "/");
        return;
    }
    if (r.kind == kDirNoIndex && rc_.shared.autoindex_enabled) {
        std::string html = build_autoindex_html(r.path, req_.path);
        res_ = HttpResponse::make_response_with_body(
            HttpResponse::kStatusOk, "text/html; charset=UTF-8", html, req_);

        out_buf_ = res_.to_string();
        fd_ = -1;
        return;
    }

    struct stat file_stat;
    fd_ = open(r.path.data(), O_RDONLY);
    if (fd_ == -1) {
        set_error(HttpResponse::kStatusInternalServerError);
        return;
    }
    stat(r.path.data(), &file_stat);
    file_size_ = file_stat.st_size;
    std::string file_type = derive_file_type(r.path);
    res_ = HttpResponse::make_response_headers_only(
        HttpResponse::kStatusOk, file_type, file_size_, req_);
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

void StaticFileHandler::set_error(const HttpResponse::Status code)
{
    res_ = HttpResponse::make_error(code, rc_.shared.error_pages, req_);
    out_buf_ = res_.to_string();
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

void StaticFileHandler::set_redirect(const HttpResponse::Status code,
                                     const std::string& redirect_path)
{
    res_ = HttpResponse::make_response_headers_only(code, "", 0, req_);
    // LOG(DEBUG) << "redirect location = " << redirect_path;
    res_.location = redirect_path;
    out_buf_ = res_.to_string();
    fd_ = -1;
}
