#ifndef SERVER_HPP_
#define SERVER_HPP_

#include "Client.hpp"
#include "Socket.hpp"

#include <netinet/in.h>

#include <vector>

class Server {
public:
    Server(in_addr_t ip, uint16_t port);
    ~Server();

    void accept_client();
    void run();

private:
    Socket socket_;
    std::vector<Client*> clients_;
};

#endif // SERVER_HPP_
