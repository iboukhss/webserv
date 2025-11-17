#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(int error_code);
    virtual ~ErrorHandler();

    int getErrorCode();

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    int error_code_;
    
};

#endif
