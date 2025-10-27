

#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "Socket.hpp"

class Client {
public:
    explicit Client(Socket* socket);
    ~Client();

private:
    Client();
    Client(const Client& other);
    Client& operator=(const Client& other);

private:
    Socket* socket_;
};

#endif // CLIENT_HPP_
