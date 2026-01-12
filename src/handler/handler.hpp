#ifndef HANDLER_HANDLER_HPP_
#define HANDLER_HANDLER_HPP_

#include <cstddef>

class Handler {
public:
    virtual ~Handler() {}

    virtual size_t read_output(char* buf, size_t n) = 0;
    virtual size_t write_input(const char* buf, size_t n) = 0;

    virtual bool has_output() const = 0;
    virtual bool needs_input() const = 0;
    virtual bool is_done() const = 0;
    
    virtual int cgi_read_fd() const = 0;
    virtual int cgi_write_fd() const = 0;

protected:
    Handler() {} // I read the docs, apparently this is legit

private:
    Handler(const Handler&);
    Handler& operator=(const Handler&);
};

#endif
