#ifndef SERVER_H_
#define SERVER_H_

#include "Client.hpp"
#include "structs_dev.hpp"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <csignal>
#include <string>

#define WEBSERV_MAX_EVENTS 64

extern volatile sig_atomic_t g_sigint_received;

class Server {
public:
    Server(in_addr_t ip, in_port_t port, int limit_conn, const struct ServerConfig* config);
    ~Server();

    void run();
    void add_connection(Client* conn);
    void remove_connection(Client* conn);
    void accept_connection();
    void send_response(Client* conn);

    int fd() { return fd_; }

    void send_error(Client* conn, int error_code, std::string body);
    const char* reason_phrase(int error_code);
    void handle_request(Client* conn, const HttpRequest& request);
    const Location* routing(const std::string& location); // later pass byRef HttpRequest object,
    int prefix_length(const std::string& req_location, const std::string& location);
    std::string build_request_path(const HttpRequest& location, const Location* request_path);
    int valid_method(const std::string& method);

private:
    Server(const Server& other);
    Server& operator=(const Server& other);

private:
    int fd_;
    sockaddr_in addr_;
    int epoll_fd_;
    epoll_event events_[WEBSERV_MAX_EVENTS];
    Client* list_head_;
    const struct ServerConfig* config_;
};

#endif
