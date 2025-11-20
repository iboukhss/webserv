#ifndef CORE_SERVER_H_
#define CORE_SERVER_H_

#include "config/server_config.hpp"
#include "core/client.hpp"
#include "router/router.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define WEBSERV_MAX_EVENTS 64

class Server {
public:
    explicit Server(const ServerConfig& config);
    ~Server();

    void init();
    void run();

    int fd() const { return fd_; }

private:
    Server(const Server& other);
    Server& operator=(const Server& other);

    void accept_connection();
    void handle_events(Client& conn, uint32_t events);

    uint64_t add_connection(int client_fd, const sockaddr_in& addr);
    void remove_connection(uint64_t id);

    Client& get_client(uint64_t id);

    const ServerConfig config_;

    int fd_;
    sockaddr_in addr_;
    int epoll_fd_;
    epoll_event events_[WEBSERV_MAX_EVENTS];
    std::map<uint64_t, Client*> clients_;
    uint64_t next_id_;
    Router router_;
};

#endif // CORE_SERVER_HPP_
