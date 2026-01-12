#include "handler/delete_handler.hpp"

#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

DeleteHandler::DeleteHandler(const std::string& path, const RouteConfig& rc, const HttpRequest &req)
    : path_(path), rc_(rc), req_(req), out_off_(0)
{
    struct stat file_stat;
    if (stat(path.data(), &file_stat) != 0 || !S_ISREG(file_stat.st_mode)) {
        set_error(HttpResponse::kStatusNotFound);
        return;
    }
    int n = remove(path.c_str());
    if (n == -1) {
        set_error(HttpResponse::kStatusInternalServerError);
        return;
    }
    res_ = HttpResponse::make_response_headers_only(HttpResponse::kStatusNoContent,
                                                    "text/html; charset=UTF-8", 0, req_);
    out_buf_ = res_.to_string();
}

DeleteHandler::~DeleteHandler()
{
}

size_t DeleteHandler::read_output(char* buf, size_t n)
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

// We never write to this handler (read-only)
size_t DeleteHandler::write_input(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}

void DeleteHandler::set_error(const HttpResponse::Status code)
{
    res_ = HttpResponse::make_error(code, rc_.shared.error_pages, req_);
    out_buf_ = res_.to_string();
}