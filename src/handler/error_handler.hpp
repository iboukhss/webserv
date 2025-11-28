#ifndef HANDLER_ERROR_HANDLER_HPP_
#define HANDLER_ERROR_HANDLER_HPP_

#include "handler/handler.hpp"
#include "http/http_response.hpp"

#include <sys/stat.h>

class ErrorHandler : public Handler {
public:
    explicit ErrorHandler(HttpResponse::Status code);
    virtual ~ErrorHandler();

    virtual int read_data(char* buf, int n);
    virtual int write_data(const char* buf, int n);

    virtual bool has_output() const { return (!res_sent_); };
    virtual bool needs_input() const { return (false); };
    virtual bool is_done() const { return (res_sent_); };

    int error_code() { return res_.code; };

private:
    ErrorHandler(const ErrorHandler&);
    ErrorHandler& operator=(const ErrorHandler&);

    HttpResponse res_;
    bool res_sent_;
};

#endif
