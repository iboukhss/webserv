

#include "Socket.hpp"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <stdexcept>

Socket::Socket(int domain, int type, int protocol)
{
    fd_ = socket(domain, type, protocol);
    if (fd_ == -1) {
        throw std::runtime_error(std::string("socket() failed: ") + strerror(errno));
    }
}

Socket::Socket(int fd, const sockaddr_in& addr)
    : fd_(fd),
      addr_(addr)
{
}

Socket::~Socket()
{
    if (fd_ != -1) {
        close(fd_);
    }
}

int Socket::bind(const sockaddr_in& addr)
{
    int err = ::bind(fd_, (const sockaddr*) &addr, sizeof(addr));
    if (err) {
        return err;
    }

    addr_ = addr;
    return 0;
}

int Socket::listen(int backlog)
{
    return ::listen(fd_, backlog);
}

Socket* Socket::accept()
{
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int fd = ::accept(fd_, (sockaddr*) &addr, &addr_len);
    if (fd == -1) {
        return NULL;
    }

    return new Socket(fd, addr);
}

ssize_t Socket::recv(void* buf, size_t size, int flags)
{
    return ::recv(fd_, buf, size, flags);
}

ssize_t Socket::send(const void* buf, size_t size, int flags)
{
    return ::send(fd_, buf, size, flags);
}

int Socket::set_nonblocking()
{
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }

    return 0;
}
