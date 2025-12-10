#include "core/server.hpp"

#include "core/client.hpp"
#include "core/signals.hpp"
#include "http/http_parser.hpp"
#include "http/http_version.hpp"
#include "router/router.hpp"
#include "util/log_message.hpp"
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

#include <cassert>
#include <cstring>
#include <iostream>
#include <stdexcept>

Server::Server(const ServerConfig& config)
    : config_(config),
      epoll_fd_(-1),
      next_id_(1),
      router_(config)
{
}

Server::~Server()
{
    for (std::map<int, Client*>::const_iterator it = clients_.begin(); it != clients_.end(); ++it) {
        delete it->second;
    }
    for (size_t i = 0; i < listen_fds_.size(); i++) {
        close(listen_fds_[i]);
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
}

void Server::init()
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1)
        throw std::runtime_error("epoll_create1 failed");

    for (size_t i = 0; i < config_.listen_addrs.size(); i++) {
        int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (fd == -1)
            throw std::runtime_error("socket failed");

        int yes = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        const sockaddr_in& addr = config_.listen_addrs[i];

        if (bind(fd, (sockaddr*) &addr, sizeof(addr)) == -1)
            throw std::runtime_error("bind failed");

        if (listen(fd, config_.backlog) == -1)
            throw std::runtime_error("listen failed");

        listen_fds_.push_back(fd);

        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");
    }
}

void Server::close_connection(Client& conn)
{
    int client_fd = conn.fd();

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn.fd(), NULL);

    std::map<int, Client*>::iterator it = clients_.find(client_fd);
    if (it == clients_.end()) {
        LOG(WARN) << "Attempted to remove inexistant client id";
        return;
    }

    delete it->second;
    clients_.erase(client_fd);

    LOG(DEBUG) << "Client #" << client_fd << ": connection closed";
}

Client& Server::get_client(int fd)
{
    std::map<int, Client*>::iterator it = clients_.find(fd);
    if (it == clients_.end()) {
        throw std::runtime_error("Attempted to retrieve inexistant client id");
    }
    return *(it->second);
}

// extracts the  first  connection  request  on  the  queue
// of pending connections for the listening socket
void Server::accept_connection(int fd)
{
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int client_fd = accept4(fd, (sockaddr*) &addr, &addr_len, SOCK_NONBLOCK);

    // This is no big deal, just retry another time
    if (client_fd == -1) {
        LOG(WARN) << "Client refused";
        return;
    }

    Client* client = new Client(client_fd);
    clients_[client_fd] = client;

    epoll_event ev;
    ev.events = EPOLLRDHUP | EPOLLIN;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

    LOG(DEBUG) << "Client #" << client_fd << ": connection accepted";
}

bool Server::is_listen_fd(int fd) const
{
    for (size_t i = 0; i < listen_fds_.size(); i++) {
        if (fd == listen_fds_[i])
            return true;
    }
    return false;
}

void Server::run()
{
    LOG(INFO) << "Server started!";

    while (!g_sigint_received) {
        int n_events = epoll_wait(epoll_fd_, events_, WEBSERV_MAX_EVENTS, -1);
        if (n_events == -1) {
            switch (errno) {
            case EINTR:
                // epoll_wait was interrupted by a signal, just move on as if
                // nothing happened.
                n_events = 0;
                break;
            default: throw std::runtime_error("epoll_wait failed");
            }
        }

        for (int i = 0; i < n_events; ++i) {
            uint32_t ev = events_[i].events;
            int fd = events_[i].data.fd;

            if (is_listen_fd(fd)) {
                accept_connection(fd);
            }
            else {
                Client& client = get_client(fd);
                handle_events(client, ev);
            }
        }
    }
    LOG(INFO) << "Shutting down server...";
}

// TODO: get rid of this
static void print_raw_data(const char* s, size_t n)
{
    std::cout << "< ";

    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (c == '\r') {
            std::cout << "\\r";
        }
        else if (c == '\n') {
            std::cout << "\\n\n";
            if (i < n - 1) {
                std::cout << "< ";
            }
        }
        else {
            std::cout.put(c);
        }
    }
    std::cout.flush();
}

static void log_request_info(uint64_t client_id, const HttpRequest& request)
{
    LOG(INFO) << "Client #" << client_id << ": " << request.method << " " << request.path << " "
              << http_version_to_string(request.http_version);
}

void Server::read_from_socket(Client& conn)
{
    if (conn.state() != Client::kReceivingHeaders && conn.state() != Client::kReceivingBody)
        return;

    char tmp[4096];
    ssize_t bytes_received = recv(conn.fd(), tmp, sizeof(tmp), 0);

    if (bytes_received == 0) {
        LOG(INFO) << "Client #" << conn.fd() << ": connection closed by remote client";
        conn.set_state(Client::kClosingConnection);
        return;
    }

    HttpParser& parser = conn.parser();
    parser.append_data(tmp, bytes_received);
    print_raw_data(tmp, bytes_received);

    if (conn.state() == Client::kReceivingHeaders) {

        if (parser.state() == HttpParser::kParsingError) {
            LOG(ERROR) << "Client #" << conn.fd() << ": invalid HTTP request";
            conn.set_state(Client::kClosingConnection);
            return;
        }

        if (parser.state() < HttpParser::kParsingBody) {
            return;
        }
        // We parsed headers and might have some or all body bytes?
        log_request_info(conn.fd(), parser.request());
        conn.set_handler(router_.handle_request(parser.request()));

        if (conn.handler()->needs_input()) {
            conn.set_state(Client::kReceivingBody);
        }
        else {
            conn.set_state(Client::kSendingResponse);
        }
    }

    if (conn.state() == Client::kReceivingBody) {
        size_t n = 0;
        while ((n = parser.read_body_chunk(tmp, sizeof(tmp))) > 0) {
            conn.handler()->write_data(tmp, n);
        }
        if (conn.handler()->has_output()) {
            conn.set_state(Client::kSendingResponse);
        }
    }
}

void Server::write_to_socket(Client& conn)
{
    assert(conn.handler() && "Handler must exist when writing to socket");

    if (conn.state() != Client::kSendingResponse)
        return;

    char tmp[4096];
    Handler& handler = *conn.handler();
    std::string& send_buffer = conn.send_buffer();

    if (handler.has_output()) {
        size_t n = handler.read_data(tmp, sizeof(tmp));
        send_buffer.append(tmp, n);
    }
    if (!send_buffer.empty()) {
        size_t bytes_sent = send(conn.fd(), send_buffer.data(), send_buffer.size(), 0);
        if (bytes_sent == 0) {
            LOG(WARN) << "Client #" << conn.fd() << ": failed to send any bytes!";
        }
        else {
            send_buffer.erase(0, bytes_sent);
        }
    }

    // All data has been written to socket -> either close or keep going
    if (handler.is_done() && send_buffer.empty()) {
        LOG(DEBUG) << "Client #" << conn.fd() << ": response sent";

        if (conn.keep_alive()) {
            conn.parser().reset_for_next_request();
            delete conn.handler();
            conn.set_handler(NULL);
            conn.set_state(Client::kReceivingHeaders);
        }
        else {
            conn.set_state(Client::kClosingConnection);
        }
    }
}

void Server::update_interest_list(Client& conn)
{
    epoll_event ev;
    ev.data.fd = conn.fd();

    uint32_t event_mask = EPOLLRDHUP;

    if (conn.state() == Client::kReceivingHeaders) {
        event_mask |= EPOLLIN;
        ev.events = event_mask;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn.fd(), &ev) == -1)
            throw std::runtime_error("epoll_ctl failed");

        return;
    }

    if (conn.handler()->needs_input())
        event_mask |= EPOLLIN;
    if (conn.handler()->has_output() || !conn.send_buffer().empty())
        event_mask |= EPOLLOUT;

    ev.events = event_mask;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn.fd(), &ev) == -1)
        throw std::runtime_error("epoll_ctl failed");
}

void Server::handle_events(Client& conn, uint32_t events)
{
    if (events & EPOLLRDHUP) {
        LOG(ERROR) << "Client #" << conn.fd() << ": EPOLLRDHUP - peer closed connection";
        close_connection(conn);
        return;
    }

    if (events & EPOLLIN)
        read_from_socket(conn);

    if (events & EPOLLOUT)
        write_to_socket(conn);

    if (conn.state() == Client::kClosingConnection) {
        close_connection(conn);
        return;
    }

    update_interest_list(conn);
}
