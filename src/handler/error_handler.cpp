#include "handler/error_handler.hpp"

#include "util/log_message.hpp"

#include <cstring>
#include <iostream>

int ErrorHandler::read_data(char* buf, int n)
{
    std::string res = res_.to_string();
    std::memcpy(buf, &res, n);
    return (res.size());
}

// We never write to this handler (read-only)
int ErrorHandler::write_data(const char* buf, int n)
{
    (void) buf;
    (void) n;
    return 0;
}

ErrorHandler::ErrorHandler(HttpResponse::Status code)
    : res_sent_(false)
{
    res_.code = code;
    LOG(DEBUG) << "ErrorHandler constructed with code " << code;
}

ErrorHandler::~ErrorHandler()
{
}
