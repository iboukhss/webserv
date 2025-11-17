#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(int error_code);
    virtual ~ErrorHandler();

    virtual bool is_readable() const { return !is_done(); } // read until done
    virtual bool is_writable() const { return false; };     // handler is read-only
    virtual bool is_done() const { return (true); };

    int get_error_code();

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    int error_code_;
};

#endif
