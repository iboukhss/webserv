#include "core/client.hpp"

#include "util/log_message.hpp"
#include "util/syscall_error.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <iostream>

Client::Client(uint64_t id, int fd, const sockaddr_in& addr)
    : state_(kReceivingHeaders),
      id_(id),
      fd_(fd),
      addr_(addr),
      handler_(NULL)
{
    set_nonblocking();
}

Client::~Client()
{
    if (fd_ != -1) {
        close(fd_);
    }
    if (handler_) {
        delete handler_;
    }
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
