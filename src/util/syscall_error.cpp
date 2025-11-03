#include "util/syscall_error.hpp"

#include <cstring>

UnrecoverableError::UnrecoverableError(const std::string& msg)
    : std::runtime_error(msg),
      err_(-1)
{
}

UnrecoverableError::UnrecoverableError(const std::string& msg, int err)
    : std::runtime_error(msg + ": " + std::strerror(err)),
      err_(err)
{
}
