#include "core/server.hpp"

#include "core/client.hpp"
#include "core/signals.hpp"
#include "http/http_parser.hpp"
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

Server::Server(const ServerConfig& config)
    : config_(config),
      fd_(-1),
      epoll_fd_(-1),
      next_id_(1),
      router_(config)
{
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

void Server::init()
{
    fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    // Make debugging less painful with bind: Address already in use
    int yes = 1;
    setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes));

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(config_.listen_port);
    addr_.sin_addr.s_addr = htonl(config_.server_ip);

    bind(fd_, (sockaddr*) &addr_, sizeof(addr_));
    listen(fd_, config_.backlog);

    epoll_fd_ = epoll_create1(0);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u64 = 0;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &ev);
}

uint64_t Server::add_connection(int client_fd, const sockaddr_in& addr)
{
    uint64_t client_id = next_id_++;
    Client* client = new Client(client_id, client_fd, addr);

    clients_[client_id] = client;

    return client_id;
}

void Server::close_connection(Client& conn)
{
    uint64_t client_id = conn.id();

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn.fd(), NULL);

    std::map<uint64_t, Client*>::iterator it = clients_.find(client_id);
    if (it == clients_.end()) {
        LOG(WARN) << "Attempted to remove inexistand client id";
        return;
    }

    delete it->second;
    clients_.erase(client_id);
    LOG(DEBUG) << "Client #" << client_id << ": connection closed";
}

Client& Server::get_client(uint64_t id)
{
    std::map<uint64_t, Client*>::iterator it = clients_.find(id);
    if (it == clients_.end()) {
        throw std::runtime_error("Attempted to retrieve inexistant client id");
    }
    return *(it->second);
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

    uint64_t client_id = add_connection(client_fd, addr);

    epoll_event ev;
    ev.events = EPOLLRDHUP | EPOLLIN;
    ev.data.u64 = client_id;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);

    LOG(DEBUG) << "Client #" << client_id << ": connection accepted";
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
            default: throw UnrecoverableError("epoll_wait", errno);
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
    LOG(INFO) << "Shutting down server...";
}

// Dirty hack

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
              << request.http_version;
}

void Server::read_from_socket(Client& conn)
{
    if (conn.state() != Client::kReceivingHeaders && conn.state() != Client::kReceivingBody)
        return;

    char tmp[4096];
    ssize_t bytes_received = recv(conn.fd(), tmp, sizeof(tmp), 0);

    if (bytes_received == 0) {
        LOG(INFO) << "Client #" << conn.id() << ": connection closed by remote client";
        conn.set_state(Client::kClosingConnection);
        return;
    }

    HttpParser& parser = conn.parser();
    parser.append_data(tmp, bytes_received);
    print_raw_data(tmp, bytes_received);

    if (conn.state() == Client::kReceivingHeaders) {

        if (parser.state() == HttpParser::kParsingError) {
            LOG(ERROR) << "Client #" << conn.id() << ": invalid HTTP request";
            conn.set_state(Client::kClosingConnection);
            return;
        }

        if (parser.state() < HttpParser::kParsingBody) {
            return;
        }
        // We parsed headers and might have some or all body bytes?
        log_request_info(conn.id(), parser.request());
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
            LOG(WARN) << "Client #" << conn.id() << ": failed to send any bytes!";
        }
        else {
            send_buffer.erase(0, bytes_sent);
        }
    }

    // All data has been written to socket -> either close or keep going
    if (handler.is_done() && send_buffer.empty()) {
        LOG(DEBUG) << "Client #" << conn.id() << ": response sent";

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

void Server::handle_events(Client& conn, uint32_t events)
{
    if (events & EPOLLRDHUP) {
        LOG(ERROR) << "Client #" << conn.id() << ": EPOLLRDHUP - peer closed connection";
        close_connection(conn);
        return;
    }
    if (events & EPOLLIN) {
        read_from_socket(conn);
    }
    if (events & EPOLLOUT) {
        write_to_socket(conn);
    }
    if (conn.state() == Client::kClosingConnection) {
        close_connection(conn);
        return;
    }

    epoll_event ev;
    ev.data.u64 = conn.id();

    uint32_t event_mask = EPOLLRDHUP;

    if (conn.state() == Client::kReceivingHeaders) {
        event_mask |= EPOLLIN;
        ev.events = event_mask;
        epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn.fd(), &ev);
        return;
    }

    if (conn.handler()->needs_input())
        event_mask |= EPOLLIN;
    if (conn.handler()->has_output() || !conn.send_buffer().empty())
        event_mask |= EPOLLOUT;

    ev.events = event_mask;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn.fd(), &ev);
}
