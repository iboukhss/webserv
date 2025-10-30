

#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "Socket.hpp"

class Client {
public:
    explicit Client(Socket* socket);
    ~Client();

    Socket* socket() { return socket_; }
    Client* next() { return next_; }
    Client* prev() { return prev_; }

    void set_next(Client* conn) { next_ = conn; }
    void set_prev(Client* conn) { prev_ = conn; }

private:
    Client();
    Client(const Client& other);
    Client& operator=(const Client& other);

private:
    Socket* socket_;
    Client* next_;
    Client* prev_;
};

#endif // CLIENT_HPP_
