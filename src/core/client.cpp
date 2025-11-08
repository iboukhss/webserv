#include "core/client.hpp"

#include "util/syscall_error.hpp"

#include <fcntl.h>
#include <unistd.h>

void Client::set_handler(Handler* handler)
{
    handler_ = handler;
}

// send buffer, if buffer empty then cal
void Client::on_write()
{
}

Client::Client(int fd, const sockaddr_in& addr)
    : fd_(fd),
      addr_(addr),
      prev_(NULL),
      next_(NULL),
      handler_(NULL)

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
        throw UnrecoverableError("fcntl", errno);
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw UnrecoverableError("fcntl", errno);
    }
}
