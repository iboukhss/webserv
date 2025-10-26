

// singleton ?

#ifndef SERVER_H_
#define SERVER_H_

#include "Client.hpp"
#include "Socket.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <map>

#define MAX_EVENTS 10

class Server {
public:
    Server(in_addr_t ip, in_port_t port, int limit_conn);
    ~Server();

    void run();
    void accept_connection();
    void send_response(int event_fd);

    int listen_fd() { return socket_.fd(); }

private:
    Server();

private:
    Socket socket_;
    int epoll_fd_;
    epoll_event events_[MAX_EVENTS];
    std::map<int, Client*> clients_; // Map of clients connections
};

#endif // SERVER_H_
