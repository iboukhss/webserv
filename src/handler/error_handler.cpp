#include "handler/error_handler.hpp"

#include "util/log_message.hpp"

#include <unistd.h>

#include <cstring>
#include <iostream>

size_t ErrorHandler::read_data(char* buf, size_t n)
{
    std::string res = res_.to_string();
    LOG(DEBUG) << res;
    size_t to_copy = std::min(res.size(), n);
    std::memcpy(buf, res.data(), to_copy);
    return (to_copy);
}

// We never write to this handler (read-only)
size_t ErrorHandler::write_data(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}

ErrorHandler::ErrorHandler(HttpResponse::Status code)
    : res_sent_(false),
      read_fd_(-1),
      write_fd_(-1)
{
    res_.code = code;
    res_.inline_body = "<h1> some error code to be shown here <h1>";
    LOG(DEBUG) << "ErrorHandler constructed with code " << code;
}

ErrorHandler::~ErrorHandler()
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
