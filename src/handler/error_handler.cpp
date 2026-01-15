#include "handler/error_handler.hpp"

#include "util/log_message.hpp"

#include <cstring>
#include <iostream>

ErrorHandler::ErrorHandler(const HttpResponse::Status code, const RouteConfig& rc,
                           const HttpRequest& req)
    : rc_(rc),
      req_(req),
      out_off_(0)
{
    res_ = HttpResponse::make_error(code, rc.shared.error_pages, req_);
    out_buf_ = res_.to_string();
    LOG(DEBUG) << "ERROR HANDLER CONSTRUCTOR";
}

ErrorHandler::~ErrorHandler()
{
}

size_t ErrorHandler::read_output(char* buf, size_t n)
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
size_t ErrorHandler::write_input(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}
