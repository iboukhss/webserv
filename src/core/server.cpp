#include "core/server.hpp"

#include "core/signals.hpp"
#include "handler/file_handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"
#include "router/router.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

Server::Server(in_addr_t ip, in_port_t port, int limit_conn, ServerConfig& config)
    : config_(config),
      router_(config)
{
    fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ == -1) {
        throw UnrecoverableError("socket", errno);
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(ip);

    if (::bind(fd_, (sockaddr*) &addr_, sizeof(addr_)) == -1) {
        throw UnrecoverableError("bind", errno);
    }

    if (::listen(fd_, limit_conn) == -1) {
        throw UnrecoverableError("listen", errno);
    }

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        throw UnrecoverableError("epoll_create1", errno);
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &ev) == -1) {
        throw UnrecoverableError("epoll_ctl", errno);
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

    std::cout << "* Connection established\n";

    Client* client = new Client(client_fd, addr);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = client;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        throw UnrecoverableError("epoll_ctl", errno);
    }

    add_connection(client);
}

static void print_http_request(const std::string& s)
{
    std::cout << "< ";

    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\r') {
            std::cout << "\\r";
        }
        else if (c == '\n') {
            std::cout << "\\n\n";
            if (i < s.size() - 1) {
                std::cout << "< ";
            }
        }
        else {
            std::cout.put(c);
        }
    }
    std::cout.flush();
}

void Server::run()
{
    while (!g_sigint_received) {
        int n_events = epoll_wait(epoll_fd_, events_, WEBSERV_MAX_EVENTS, -1);
        if (n_events == -1) {
            switch (errno) {
            case EINTR:
                // epoll_wait was interrupted by a signal, just move on as if
                // nothing happened.
                n_events = 0;
                break;
            default:
                throw UnrecoverableError("epoll_wait", errno);
            }
        }

        for (int i = 0; i < n_events; ++i) {
            Client* conn = (Client*) events_[i].data.ptr;

            // We set ev.data.ptr to NULL in the constructor, so we know for
            // sure this notification is coming from the server socket.
            if (conn == NULL) {
                accept_connection();
            }
            else {
                handle_events(conn, events_[i].events);
            }
        }
    }
}

void Server::handle_events(Client* conn, uint32_t events)
{
    if (events & EPOLLIN) {
        receive_request(conn);
    }
    if (events & EPOLLOUT) {
        send_response(conn);
    }
}

// also chunked requests will need to be handled
void Server::receive_request(Client* conn)
{
    int client_fd = conn->fd();
    char buf[4096];

    int n_bytes = recv(client_fd, buf, sizeof(buf), 0);
    conn->recv_buffer().append(buf, n_bytes);

    std::cout << "* Request received\n";

    print_http_request(conn->recv_buffer());

    epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.ptr = conn;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);

    // reading from socket and storing request in Client (TO DO : IBOUKHSS)
    // for dev, mimic a http request and call the routing function
    HttpRequest req;
    req.method = "GET";
    req.path = "/files/42.txt";

    conn->set_request(req);

    // DHE: adding full_path variable to run make lint. The router_.handle_request() call needs to
    // be moved out
    std::string full_path;
    if (!router_.handle_request(conn, req, full_path))
        return; // bad request
    if (req.method = "GET") {
        // add handler to conn
    }
}

void Server::send_response(Client* conn)
{
    int client_fd = conn->fd();
    HttpResponse res = conn->res();
    std::string raw = res.to_string();

    send(client_fd, raw.data(), raw.size(), 0);

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
        throw UnrecoverableError("epoll_ctl", errno);
    }

    remove_connection(conn);
}
