#include "handler/redirect_handler.hpp"

#include "http/http_response.hpp"
#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <cstring>

RedirectHandler::RedirectHandler(const RouteConfig& rc, const HttpRequest& req)
    : rc_(rc),
      req_(req),
      out_off_(0)
{
    res_ = HttpResponse::make_response_headers_only(rc_.shared.redirect.code, "", 0, req_);
    res_.location = rc_.shared.redirect.url;
    out_buf_ = res_.to_string();
}
RedirectHandler::~RedirectHandler()
{
}

size_t RedirectHandler::read_output(char* buf, size_t n)
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
size_t RedirectHandler::write_input(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}
