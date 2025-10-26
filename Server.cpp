

#include "Server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

    epoll_event ev;

    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd();
    epoll_fd_ = epoll_create1(0);

    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd(), &ev);
}

Server::~Server()
{
    // do I need to close the socket ?
    // not anymore :P

    for (std::map<int, Client*>::iterator it = clients_.begin(); it != clients_.end(); ++it) {
        delete it->second;
    }

    close(epoll_fd_);
}

void Server::run()
{
    while (true) {
        // poll for available connections
        // two solutions possible : wait returns n events -> either loop 0 to n in the epoll_event
        // array held by the epoll object or modify the wait function to return an array
        int n_events = epoll_wait(epoll_fd_, events_, MAX_EVENTS, -1);

        for (int i = 0; i < n_events; ++i) {
            int event_fd = events_[i].data.fd;

            // we need to accept a new connection(s) -> there might be a queue
            if (event_fd == listen_fd()) {
                accept_connection();
            }
            // we need to write to a client socket
            else {
                send_response(event_fd);
            }
        }
    }
}

// extracts the  first  connection  request  on  the  queue
// of pending connections for the listening socket
void Server::accept_connection()
{
    Socket* client_sock = socket_.accept();

    // error occured or no further connection request to be handled -> fine
    if (client_sock == NULL) {
        return;
    }

    client_sock->set_nonblocking();

    epoll_event ev;
    int client_fd = client_sock->fd();

    ev.events = EPOLLIN;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

    // save all new connections
    clients_[client_fd] = new Client(client_sock);
}

void Server::send_response(int event_fd)
{
    const char* response = "HTTP/1.1 200 OK\r\n"
                           "Content-Length: 14\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Hello, client!\n";

    send(event_fd, response, strlen(response), 0);
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event_fd, NULL);
    delete clients_[event_fd];
    clients_.erase(event_fd);
}
