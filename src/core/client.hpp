#ifndef CORE_CLIENT_HPP_
#define CORE_CLIENT_HPP_

#include "handler/handler.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <netinet/in.h>

#include <string>

class Client {
public:
    Client(uint64_t id, int fd, const sockaddr_in& addr);
    ~Client();

    // setters
    void set_handler(Handler* handler) { handler_ = handler; }
    void set_nonblocking();
    void set_request(const HttpRequest& req) { req_ = req; }
    void set_response(const HttpResponse& res) { res_ = res; }

    // getters
    uint64_t id() const { return id_; }
    int fd() const { return fd_; }
    const sockaddr_in& addr() const { return addr_; }
    const HttpRequest& req() const { return req_; }
    HttpResponse& res() { return res_; }
    std::string& recv_buffer() { return recv_buffer_; }
    std::string& send_buffer() { return send_buffer_; }
    Handler* handler() const { return handler_; }

private:
    Client();
    Client(const Client& other);
    Client& operator=(const Client& other);

private:
    uint64_t id_;
    int fd_;
    sockaddr_in addr_;
    HttpRequest req_;
    HttpResponse res_; // TODO: remove
    std::string recv_buffer_;
    std::string send_buffer_;
    Handler* handler_;
};

#endif // CORE_CLIENT_HPP_
