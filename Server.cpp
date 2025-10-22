

#include "Server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

int Server::set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return (-1);
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {

        return (-1);
    }
    return (0);
}

int Server::getFd()
{
    return (server_fd_);
}

Server::Server(int limit_conn)
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        exit(EXIT_FAILURE);
    }

    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_port = htons(8080);
    serverAddress_.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd_, (struct sockaddr*)&serverAddress_, sizeof(serverAddress_));
    set_nonblocking(server_fd_);
    listen(server_fd_, limit_conn);
}

Server::~Server()
{
}
