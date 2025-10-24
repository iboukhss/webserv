#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include <netinet/in.h>
#include <sys/socket.h>

class Socket {
public:
    Socket(int domain, int type, int protocol);
    Socket(int fd, const sockaddr_in& addr);
    ~Socket();

    void bind(const sockaddr_in& addr);
    void listen(int backlog);
    Socket* accept();
    ssize_t recv(void* buf, size_t size, int flags = 0);
    ssize_t send(const void* buf, size_t size, int flags = 0);

    int fd() const { return fd_; }

private:
    int fd_;
    sockaddr_in addr_;
};

#endif // SOCKET_HPP_
