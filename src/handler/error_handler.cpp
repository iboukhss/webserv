#include "handler/error_handler.hpp"

#include "util/log_message.hpp"

#include <cstring>
#include <iostream>

size_t ErrorHandler::read_data(char* buf, size_t n)
{
    std::string res = res_.to_string();
    LOG(DEBUG) << res;
    std::memcpy(buf, &res, n);
    return (res.size());
}

// We never write to this handler (read-only)
size_t ErrorHandler::write_data(const char* buf, size_t n)
{
    (void) buf;
    (void) n;
    return 0;
}

ErrorHandler::ErrorHandler(HttpResponse::Status code)
    : res_sent_(false)
{
    res_.http_version = "HTTP/1.1";
    res_.code = code;
    res_.inline_body = "<h1> some error code to be shown here <h1>";
    LOG(DEBUG) << "ErrorHandler constructed with code " << code;
}

ErrorHandler::~ErrorHandler()
{
}
