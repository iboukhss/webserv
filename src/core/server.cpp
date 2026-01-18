#include "core/server.hpp"

#include "core/client.hpp"
#include "core/server_defaults.hpp"
#include "core/signals.hpp"
#include "core/virtual_server.hpp"
#include "util/log_message.hpp"

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
#include <cerrno>
#include <cstring>
#include <stdexcept>

Server::Server(const HttpConfig& config)
    : config_(config),
      epoll_fd_(-1)
{
    std::memset(events_, 0, sizeof(events_));
}

Server::~Server()
{
    for (size_t i = 0; i < clients_.size(); i++) {
        delete clients_[i];
    }
    for (size_t i = 0; i < listen_fds_.size(); i++) {
        close(listen_fds_[i]);
    }
    for (size_t i = 0; i < vservers_.size(); i++) {
        delete vservers_[i];
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
}

void Server::init()
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1)
        throw std::runtime_error("Server::init: epoll_create1 failed");

    for (size_t i = 0; i < config_.servers.size(); i++) {
        const ServerConfig& sconf = config_.servers[i];

        VirtualServer* vs = new VirtualServer(sconf);
        vservers_.push_back(vs);

        for (size_t j = 0; j < sconf.listen_addrs.size(); j++) {
            int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
            if (fd == -1)
                throw std::runtime_error("Server::init: socket failed");

            int yes = 1;
            setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

            const sockaddr_in& addr = sconf.listen_addrs[j];

            if (bind(fd, (sockaddr*) &addr, sizeof(addr)) == -1)
                throw std::runtime_error("Server::init: bind failed");

            if (listen(fd, WEBSERV_DEFAULT_MAX_PENDING_CONNECTIONS) == -1)
                throw std::runtime_error("Server::init: listen failed");

            listen_fds_.push_back(fd);
            server_map_[fd] = vs;

            epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fd;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1)
                throw std::runtime_error("Server::init: EPOLL_CTL_ADD failed");
        }
    }
}

// extracts the  first  connection  request  on  the  queue
// of pending connections for the listening socket
void Server::accept_connection(int fd)
{
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int client_fd = accept4(fd, (sockaddr*) &addr, &addr_len, SOCK_NONBLOCK);
    if (client_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        throw std::runtime_error("Server::accept_connection: accept4 failed");
    }

    VirtualServer* vs = server_map_[fd];
    Client* client = new Client(client_fd, *vs);
    clients_.push_back(client);
    client_map_[client_fd] = client;

    epoll_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLRDHUP | EPOLLIN;
    ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        throw std::runtime_error("Server::accept_connection: EPOLL_CTL_ADD failed");
    }

    LOG(DEBUG) << "Client #" << client_fd << ": connection accepted";
}

void Server::close_connection(Client& client)
{
    int client_fd = client.fd();
    std::map<int, uint32_t> fds = client.epoll_fds();

    for (std::map<int, uint32_t>::const_iterator it = fds.begin(); it != fds.end(); ++it) {
        int fd = it->first;

        if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) == -1)
            throw std::runtime_error("Server::close_connection: EPOLL_CTL_DEL failed");

        client_map_.erase(fd);
    }

    for (std::vector<Client*>::iterator it = clients_.begin(); it != clients_.end(); ++it) {
        if (*it == &client) {
            clients_.erase(it);
            break;
        }
    }

    delete &client;

    LOG(DEBUG) << "Client #" << client_fd << ": connection closed";
}

Client& Server::get_client(int fd)
{
    if (client_map_.count(fd) == 0)
        throw std::runtime_error("Server::get_client: Attempted to retrieve invalid client fd");

    return *client_map_[fd];
}

bool Server::is_listen_fd(int fd) const
{
    for (size_t i = 0; i < listen_fds_.size(); i++) {
        if (fd == listen_fds_[i])
            return true;
    }
    return false;
}

bool Server::is_client_fd(int fd) const
{
    return client_map_.find(fd) != client_map_.end();
}

void Server::run()
{
    LOG(INFO) << "Server started!";

    while (!g_sigint_received) {
        int n_events = epoll_wait(epoll_fd_, events_, WEBSERV_MAX_EVENTS, -1);
        if (n_events == -1) {
            if (errno == EINTR) {
                continue;
            }
            throw std::runtime_error("Server::run: epoll_wait failed");
        }

        for (int i = 0; i < n_events; ++i) {
            uint32_t ev = events_[i].events;
            int fd = events_[i].data.fd;

            if (is_listen_fd(fd)) {
                accept_connection(fd);
            }
            else if (is_client_fd(fd)) {
                handle_events(fd, ev);
            }
            else {
                LOG(WARN) << "File descriptor: " << fd << " not tracked anymore";
                continue;
            }
        }
    }
    LOG(INFO) << "Shutting down server...";
}

void Server::handle_events(int fd, uint32_t events)
{
    Client& client = get_client(fd);

    if (events & EPOLLIN) {
        client.on_epollin(fd);
    }
    if (events & EPOLLOUT) {
        client.on_epollout(fd);
    }

    if (fd == client.fd() && events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        LOG(ERROR) << "Client #" << client.fd() << ": EPOLLRDHUP - peer closed connection";
        close_connection(client);
        return;
    }

    if (client.should_close()) {
        close_connection(client);
        return;
    }

    update_interest_list(client);
}

void Server::update_interest_list(Client& client)
{
    std::map<int, uint32_t> fds = client.epoll_fds();

    for (std::map<int, uint32_t>::const_iterator it = fds.begin(); it != fds.end(); ++it) {
        int fd = it->first;
        uint32_t mask = it->second;

        epoll_event ev;
        ev.data.fd = fd;
        ev.events = mask;

        if (client_map_.count(fd) == 0) {

            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
                throw std::runtime_error("Server::update_interest_list: EPOLL_CTL_ADD failed");
            }

            client_map_[fd] = &client;
        }
        else {
            if (mask == 0) {
                /*
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev) == -1) {
                    throw std::runtime_error("Server::update_interest_list: EPOLL_CTL_DEL failed");
                }
                */
                client_map_.erase(fd);
            }
            else {
                if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
                    throw std::runtime_error("Server::update_interest_list: EPOLL_CTL_MOD failed");
                }
            }
        }
    }
    client.on_sync_complete();
}
