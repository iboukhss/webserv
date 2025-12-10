#ifndef CORE_CLIENT_HPP_
#define CORE_CLIENT_HPP_

#include "handler/handler.hpp"
#include "http/http_parser.hpp"
#include "http/http_request.hpp"
#include "http/http_response.hpp"

#include <netinet/in.h>

#include <string>

class Client {
public:
    enum State { kClosingConnection = 0, kReceivingHeaders, kReceivingBody, kSendingResponse };

    explicit Client(int fd);
    ~Client();

    // setters
    void set_handler(Handler* handler) { handler_ = handler; }
    void set_nonblocking();
    void set_state(Client::State state) { state_ = state; }

    // getters
    Client::State state() { return state_; }
    int fd() const { return fd_; }
    bool keep_alive() { return parser_.request().keep_alive; }
    std::string& send_buffer() { return send_buffer_; }
    HttpParser& parser() { return parser_; }
    Handler* handler() const { return handler_; }

private:
    Client();
    Client(const Client& other);
    Client& operator=(const Client& other);

private:
    Client::State state_;
    uint64_t id_;
    int fd_;
    sockaddr_in addr_;
    std::string send_buffer_;
    HttpParser parser_;
    Handler* handler_;
};

#endif // CORE_CLIENT_HPP_
