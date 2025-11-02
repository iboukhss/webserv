#ifndef SYSCALLERROR_HPP_
#define SYSCALLERROR_HPP_

#include <errno.h>

#include <stdexcept>

class SyscallError : public std::runtime_error {
public:
    SyscallError(const std::string& msg, int err);

    int code() const { return err_; }

private:
    int err_;
};

#endif // SYSCALLERROR_HPP_
