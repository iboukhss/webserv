#ifndef CORE_SERVER_H_
#define CORE_SERVER_H_

#include "config/server_config.hpp"
#include "core/client.hpp"
#include "http/http_request.hpp"
#include "router/router.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define WEBSERV_MAX_EVENTS 64

class Server {
public:
    Server(in_addr_t ip, in_port_t port, int limit_conn, ServerConfig& config);
    ~Server();

    void run();

    void handle_events(Client* conn, uint32_t events);

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
    Router router_;
};

#endif // CORE_SERVER_HPP_
