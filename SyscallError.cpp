#include "SyscallError.hpp"

#include <cstring>

SyscallError::SyscallError(const std::string& msg, int err)
    : std::runtime_error(msg + ": " + std::strerror(err)),
      err_(err)
{
}
