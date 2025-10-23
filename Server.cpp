

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

int Server::acceptConn()
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr,
                           &client_len); // extracts the  first  connection  request  on  the  queue
                                         // of pending connections for the listening socket
    if (client_fd == -1) {
        // error occured or no further connection request to be handled -> fine
        return (-1);
    }
    set_nonblocking(client_fd);
    return (client_fd);
}

void Server::sendResponse(int event_fd, const char* response)
{
    send(event_fd, response, strlen(response), 0);
}

Server::Server(int port, int limit_conn)
{
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        exit(EXIT_FAILURE);
    }

    serverAddress_.sin_family = AF_INET;
    serverAddress_.sin_port = htons(port);
    serverAddress_.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd_, (struct sockaddr*)&serverAddress_, sizeof(serverAddress_));
    set_nonblocking(server_fd_);
    listen(server_fd_, limit_conn);
}

Server::~Server()
{
    // do I need to close the socket ?
}
