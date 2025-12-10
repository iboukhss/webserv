#include "core/client.hpp"

#include <fcntl.h>
#include <unistd.h>

Client::Client(int fd)
    : state_(kReceivingHeaders),
      fd_(fd),
      handler_(NULL)
{
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
