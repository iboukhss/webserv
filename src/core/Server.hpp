#ifndef CORE_SERVER_H_
#define CORE_SERVER_H_

#include "config/ServerConfig.hpp"
#include "core/Client.hpp"
#include "http/HttpRequest.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <csignal>

#define WEBSERV_MAX_EVENTS 64

extern volatile sig_atomic_t g_sigint_received;

class Server {
public:
    Server(in_addr_t ip, in_port_t port, int limit_conn, ServerConfig& config);
    ~Server();

    void run();

    void handle_events(Client* conn, uint32_t events);
    void handle_request(Client* conn, const HttpRequest& request);

    void accept_connection();
    void add_connection(Client* conn);
    void remove_connection(Client* conn);

    void receive_request(Client* conn);
    void send_response(Client* conn);

    int fd() { return fd_; }

private:
    Server(const Server& other);
    Server& operator=(const Server& other);

private:
    ServerConfig config_; // I agree, the config should be const, just a temp change
    int fd_;
    sockaddr_in addr_;
    int epoll_fd_;
    epoll_event events_[WEBSERV_MAX_EVENTS];
    Client* list_head_;
};

#endif // CORE_SERVER_HPP_
