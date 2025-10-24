#include "Socket.hpp"

#include <stdexcept>

#include <errno.h>
#include <string.h>
#include <unistd.h>

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

void Socket::bind(const sockaddr_in& addr)
{
    if (::bind(fd_, (const sockaddr*) &addr, sizeof(addr)) == -1) {
        throw std::runtime_error(std::string("bind() failed: ") + strerror(errno));
    }

    addr_ = addr;
}

void Socket::listen(int backlog)
{
    if (::listen(fd_, backlog) == -1) {
        throw std::runtime_error(std::string("listen() failed: ") + strerror(errno));
    }
}

Socket* Socket::accept()
{
    sockaddr_in addr;

    int fd = ::accept(fd_, (sockaddr*) &addr, NULL);
    if (fd == -1) {
        throw std::runtime_error(std::string("accept() failed: ") + strerror(errno));
    }
    return new Socket(fd, addr);
}

ssize_t Socket::recv(void* buf, size_t size, int flags)
{
    ssize_t bytes_received = ::recv(fd_, buf, size, flags);
    if (bytes_received == -1) {
        throw std::runtime_error(std::string("recv() failed: ") + strerror(errno));
    }
    return bytes_received;
}

ssize_t Socket::send(const void* buf, size_t size, int flags)
{
    ssize_t bytes_sent = ::send(fd_, buf, size, flags);
    if (bytes_sent == -1) {
        throw std::runtime_error(std::string("send() failed: ") + strerror(errno));
    }
    return bytes_sent;
}
