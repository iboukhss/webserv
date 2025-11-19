#include "core/server.hpp"

#include "core/signals.hpp"
#include "http/http_request.hpp"
#include "router/router.hpp"
#include "util/syscall_error.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

Server::Server(in_addr_t ip, in_port_t port, int limit_conn, ServerConfig& config)
    : config_(config),
      router_(config),
      next_id_(1)
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

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u64 = 0;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &ev);
}

Server::~Server()
{
    for (std::map<uint64_t, Client*>::const_iterator it = clients_.begin(); it != clients_.end();
         ++it) {
        delete it->second;
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}

Client& Server::get_client(uint64_t id)
{
    std::map<uint64_t, Client*>::iterator it = clients_.find(id);
    if (it == clients_.end()) {
        throw std::runtime_error("[FATAL] Attempted to retrieve inexistant client id");
    }
    return *(it->second);
}

void Server::remove_connection(uint64_t id)
{
    std::map<uint64_t, Client*>::iterator it = clients_.find(id);
    if (it == clients_.end()) {
        std::cerr << "[WARNING] Attempted to remove inexistant client id" << std::endl;
        return;
    }
    delete it->second;
    clients_.erase(id);
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

    uint64_t client_id = next_id_++;
    Client* client = new Client(client_id, client_fd, addr);

    clients_[client_id] = client;

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u64 = client_id;

    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

    std::cout << "* Connection established, client_id = " << client_id << std::endl;
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
            uint32_t ev = events_[i].events;
            uint64_t id = events_[i].data.u64;

            if (id == 0) {
                accept_connection();
            }
            else {
                Client& client = get_client(id);
                handle_events(client, ev);
            }
        }
    }
}

// Dirty hack
static void print_raw_data(const std::string& s)
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

void Server::handle_events(Client& conn, uint32_t events)
{
    char tmp[4096];

    // No handler yet, assume it's a new request (hardcoded for now)
    if (!conn.handler()) {
        HttpRequest req;
        req.method = "GET";
        req.path = "/files/42.txt";
        conn.set_request(req);

        Handler* h = router_.handle_request(conn.req());
        conn.set_handler(h);
    }

    // Can read from socket
    if (events & EPOLLIN) {
        int n = recv(conn.fd(), tmp, sizeof(tmp), 0);
        if (n == 0) {
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn.fd(), NULL);
            remove_connection(conn.id());
            return;
        }
        if (n > 0) {
            conn.recv_buffer().append(tmp, n);

            std::cout << "* Received from socket\n";
            print_raw_data(conn.recv_buffer());

            epoll_event ev;
            ev.events = EPOLLOUT;
            ev.data.u64 = conn.id();
            epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn.fd(), &ev);
        }
    }

    // Can write to socket
    if (events & EPOLLOUT) {
        if (conn.handler()->has_output()) {
            int n = conn.handler()->read_data(tmp, sizeof(tmp));
            if (n > 0)
                conn.send_buffer().append(tmp, n);
        }

        if (!conn.send_buffer().empty()) {
            int n = send(conn.fd(), conn.send_buffer().data(), conn.send_buffer().size(), 0);
            if (n > 0)
                conn.send_buffer().erase(0, n);
        }

        if (conn.handler()->is_done() && conn.send_buffer().empty()) {
            epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn.fd(), NULL);
            remove_connection(conn.id());
            return;
        }
    }
}
