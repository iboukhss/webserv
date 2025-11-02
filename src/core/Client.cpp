#include "core/Client.hpp"

#include "util/SyscallError.hpp"

#include <fcntl.h>
#include <unistd.h>

Client::Client(int fd, const sockaddr_in& addr)
    : fd_(fd),
      addr_(addr),
      prev_(NULL),
      next_(NULL)
{
    set_nonblocking();
}

Client::~Client()
{
    if (fd_ != -1) {
        close(fd_);
    }
}

void Client::set_nonblocking()
{
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw SyscallError("fcntl", errno);
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw SyscallError("fcntl", errno);
    }
}
