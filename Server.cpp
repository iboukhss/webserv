#include "Server.hpp"

#include "HttpResponse.hpp"
#include "SyscallError.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

// NOTE: This constructor is NOT exception safe (but that's probably fine)
Server::Server(in_addr_t ip, in_port_t port, int backlog)
{
    fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ == -1) {
        throw SyscallError("socket", errno);
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(ip);

    if (::bind(fd_, (sockaddr*) &addr_, sizeof(addr_)) == -1) {
        throw SyscallError("bind", errno);
    }

    if (::listen(fd_, backlog) == -1) {
        throw SyscallError("listen", errno);
    }

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        throw SyscallError("epoll_create1", errno);
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &ev) == -1) {
        throw SyscallError("epoll_ctl", errno);
    }

    list_head_ = NULL;
}

Server::~Server()
{
    while (list_head_) {
        Client* conn = list_head_;
        list_head_ = list_head_->next();
        delete conn;
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
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
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int client_fd = ::accept(fd_, (sockaddr*) &addr, &addr_len);
    if (client_fd == -1) {
        return;
    }

    Client* client = new Client(client_fd, addr);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = client;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        throw SyscallError("epoll_ctl", errno);
    }

    add_connection(client);
}

void Server::send_response(Client* conn)
{
    int client_fd = conn->fd();
    HttpResponse res = {200, "text/plain", "Hello, client!\n"};
    std::string raw = res.to_string();

    send(client_fd, raw.data(), raw.size(), 0);

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
        throw SyscallError("epoll_ctl", errno);
    }

    remove_connection(conn);
}
