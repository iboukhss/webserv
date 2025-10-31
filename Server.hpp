#ifndef SERVER_H_
#define SERVER_H_

#include "Client.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#define WEBSERV_MAX_EVENTS 64

class Server {
public:
    Server(in_addr_t ip, in_port_t port, int backlog);
    ~Server();

    void run();
    void add_connection(Client* conn);
    void remove_connection(Client* conn);
    void accept_connection();
    void send_response(Client* conn);

    int fd() { return fd_; }

private:
    Server(const Server& other);
    Server& operator=(const Server& other);

private:
    int fd_;
    sockaddr_in addr_;
    int epoll_fd_;
    epoll_event events_[WEBSERV_MAX_EVENTS];
    Client* list_head_;
};

#endif // SERVER_H_
