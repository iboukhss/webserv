#ifndef CORE_CLIENT_HPP_
#define CORE_CLIENT_HPP_

#include "core/virtual_server.hpp"
#include "handler/handler.hpp"
#include "http/http_parser.hpp"
#include "http/http_request.hpp"

#include <netinet/in.h>

#include <string>

class Client {
public:
    enum State {
        kClosingConnection = 0,
        kReceivingHeaders,
        kProcessingRequest,
        kPreparingNextRequest
    };

    Client(int fd, const VirtualServer& vs);
    ~Client();

    void on_epollin(int fd);
    void on_epollout(int fd);
    void on_sync_complete();

    int fd() const { return sockfd_; }
    bool should_close() const { return state_ == Client::kClosingConnection; }
    const std::map<int, uint32_t>& epoll_fds() { return epoll_fds_; }

private:
    Client(const Client& other);
    Client& operator=(const Client& other);

    size_t sendbuf_available_size() const;
    bool keep_alive() const { return parser_.request().keep_alive; }

    void want_read(int fd);
    void want_write(int fd);
    void stop_read(int fd);
    void stop_write(int fd);

    void read_from_virtual_file();
    void write_to_virtual_file();

    void read_from_socket();
    void maybe_create_request_handler();
    void maybe_write_to_regular_file();
    void read_from_pipe();

    void maybe_read_from_regular_file();
    void write_to_socket();
    void write_to_pipe();

    void refresh_interest_list();
    void sync_pipe_fds_from_handler();

    int sockfd_;
    int pipefd_[2];
    const VirtualServer& vserv_;
    Client::State state_;
    std::map<int, uint32_t> epoll_fds_;
    std::string sendbuf_;
    HttpParser parser_;
    Handler* handler_;
};

#endif // CORE_CLIENT_HPP_
