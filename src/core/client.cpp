#include "core/client.hpp"

#include "util/syscall_error.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

Client::Client(int fd, const sockaddr_in& addr)
    : fd_(fd),
      addr_(addr),
      prev_(NULL),
      next_(NULL),
      handler_(NULL)

{
    set_nonblocking();
    std::cout << "Client constructor called" << std::endl;
}

Client::~Client()
{
    if (fd_ != -1) {
        close(fd_);
    }
    if (handler_) {
        delete (handler_);
    }
    std::cout << "Client destructor called" << std::endl;
}

void Client::set_nonblocking()
{
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw UnrecoverableError("fcntl", errno);
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw UnrecoverableError("fcntl", errno);
    }
}
