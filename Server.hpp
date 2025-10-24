

// singleton ?

#ifndef SERVER_H_
#define SERVER_H_

#include "Client.hpp"
#include "Socket.hpp"

#include <netinet/in.h>
#include <sys/socket.h>

#include <vector>

class Server {
public:
    Server(in_addr_t ip, in_port_t port, int limit_conn);
    ~Server();

    int accept_conn();
    void send_response(int event_fd, const char* response);

    int listen_fd() { return socket_.fd(); }

private:
    Server();

private:
    Socket socket_;
    std::vector<Client*> clients_; // vector clients connections
};

#endif // SERVER_H_
