

#include "Server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <cstring>

Server::Server(in_addr_t ip, in_port_t port, int limit_conn)
    : socket_(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)
{
    sockaddr_in addr;

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(ip);
    socket_.bind(addr);
    socket_.listen(limit_conn);
}

Server::~Server()
{
    // do I need to close the socket ?
    // not anymore :P

    for (size_t i = 0; i < clients_.size(); ++i) {
        delete clients_[i];
    }
}

// extracts the  first  connection  request  on  the  queue
// of pending connections for the listening socket
int Server::accept_conn()
{
    Socket* client_sock = socket_.accept();
    if (client_sock == NULL) {
        // error occured or no further connection request to be handled -> fine
        return (-1);
    }

    client_sock->set_nonblocking();

    // save all new connections
    Client* client = new Client(client_sock);
    clients_.push_back(client);
    return (client_sock->fd());
}

void Server::send_response(int event_fd, const char* response)
{
    send(event_fd, response, strlen(response), 0);
}
