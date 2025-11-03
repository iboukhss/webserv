#ifndef CORE_CLIENT_HPP_
#define CORE_CLIENT_HPP_

#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

#include <netinet/in.h>

#include <string>

class Client {
public:
    Client(int fd, const sockaddr_in& addr);
    ~Client();

    void set_nonblocking();
    void set_request(const HttpRequest& req) { req_ = req; }
    void set_response(const HttpResponse& res) { res_ = res; }
    void set_next(Client* conn) { next_ = conn; }
    void set_prev(Client* conn) { prev_ = conn; }

    int fd() const { return fd_; }
    const sockaddr_in& addr() const { return addr_; }

    const HttpRequest& req() const { return req_; }
    HttpResponse& res() { return res_; }
    std::string& recv_buffer() { return recv_buffer_; }
    std::string& send_buffer() { return send_buffer_; }

    Client* next() { return next_; }
    Client* prev() { return prev_; }

private:
    Client();
    Client(const Client& other);
    Client& operator=(const Client& other);

private:
    int fd_;
    sockaddr_in addr_;
    HttpRequest req_;
    HttpResponse res_;
    std::string recv_buffer_;
    std::string send_buffer_;
    Client* prev_;
    Client* next_;
};

#endif // CORE_CLIENT_HPP_
