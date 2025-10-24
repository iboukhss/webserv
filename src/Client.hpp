#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "Socket.hpp"

class Client {
public:
    Client(Socket* socket);
    ~Client();

private:
    Socket* socket_;
};

#endif // CLIENT_HPP_
