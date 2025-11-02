#ifndef UTIL_SYSCALL_ERROR_HPP_
#define UTIL_SYSCALL_ERROR_HPP_

#include <errno.h>

#include <stdexcept>

class SyscallError : public std::runtime_error {
public:
    SyscallError(const std::string& msg, int err);

    int code() const { return err_; }

private:
    int err_;
};

#endif // UTIL_SYSCALL_ERROR_HPP_
