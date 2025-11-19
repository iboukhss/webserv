#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "handler/handler.hpp"

#include <sys/stat.h>

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(int error_code);
    virtual ~ErrorHandler();

    virtual bool has_output() const { return (false); };
    virtual bool needs_input() const { return (false); };
    virtual bool is_done() const { return (true); };

    int get_error_code();

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);
    virtual int read_data(char*, int) { return (0); };
    virtual int write_data(const char*, int) { return (0); };
    int error_code_;
};

#endif
