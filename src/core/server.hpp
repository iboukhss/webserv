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

private:
    Server(const Server& other);
    Server& operator=(const Server& other);

    void accept_connection(int fd);
    void handle_events(int fd, uint32_t events);
    void read_from_socket(Client& conn);
    void write_to_socket(Client& conn);
    void update_interest_list(Client& conn);

    bool is_listen_fd(int fd) const;
    bool is_client_fd(int fd) const;
    Client& get_client(int fd);
    uint64_t add_connection(int client_fd, const sockaddr_in& addr);
    void close_connection(Client& conn);

    const ServerConfig& config_;

    int epoll_fd_;
    std::vector<VirtualServer*> vservers_;
    std::vector<int> listen_fds_;
    std::vector<Client*> clients_;
    epoll_event events_[WEBSERV_MAX_EVENTS];
    std::map<int, VirtualServer*> server_map_;
    std::map<int, Client*> client_map_;
};

#endif // CORE_SERVER_HPP_
