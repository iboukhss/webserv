#include "handler/error_handler.hpp"

int ErrorHandler::get_error_code()
{
    return (error_code_);
}

ErrorHandler::ErrorHandler(int error_code)
    : error_code_(error_code)
{
}

ErrorHandler::~ErrorHandler()
{
}
