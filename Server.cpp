

#include "Server.hpp"

#include "HttpResponse.hpp"

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
    ev.data.ptr = NULL;

    epoll_fd_ = epoll_create1(0);
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd(), &ev);
    list_head_ = NULL;
}

Server::~Server()
{
    // Close all remaining active connections
    while (list_head_) {
        Client* conn = list_head_;
        list_head_ = list_head_->next();
        delete conn;
    }

    close(epoll_fd_);
}

void Server::run()
{
    while (true) {
        int n_events = epoll_wait(epoll_fd_, events_, WEBSERV_MAX_EVENTS, -1);

        for (int i = 0; i < n_events; ++i) {
            Client* conn = (Client*) events_[i].data.ptr;

            // We set ev.data.ptr to NULL in the constructor, so we know for
            // sure this notification is coming from the server socket.
            if (conn == NULL) {
                accept_connection();
            }
            else {
                send_response(conn);
            }
        }
    }
}

void Server::add_connection(Client* conn)
{
    conn->set_next(list_head_);
    if (list_head_) {
        list_head_->set_prev(conn);
    }
    list_head_ = conn;
}

void Server::remove_connection(Client* conn)
{
    Client* prev = conn->prev();
    Client* next = conn->next();

    if (prev) {
        prev->set_next(conn->next());
    }
    if (next) {
        next->set_prev(conn->prev());
    }
    if (conn == list_head_) {
        list_head_ = next;
    }

    delete conn;
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
    Client* client = new Client(client_sock);
    int client_fd = client_sock->fd();

    ev.events = EPOLLIN;
    ev.data.ptr = client;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

    add_connection(client);
}

void Server::send_response(Client* conn)
{
    int client_fd = conn->socket()->fd();
    HttpResponse res = {200, "text/plain", "Hello, client!\n"};
    std::string raw = res.to_string();

    send(client_fd, raw.data(), raw.size(), 0);
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL);
    remove_connection(conn);
}
