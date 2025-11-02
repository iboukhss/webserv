#include "core/Server.hpp"

#include "http/HttpResponse.hpp"
#include "util/SyscallError.hpp"
#include "util/structs_dev.hpp"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

const char* Server::reason_phrase(int error_code)
{
    switch (error_code) {
    case 400:
        return "Bad request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not found";
    case 405:
        return "Method not allowed";
    case 500:
        return "Internal server error";
    default:
        return "Unknown";
    }
}

enum ErrorCodes {
    kBadRequest = 400,
    kForbidden = 403,
    kNotFound = 404,
    kMethodNotAllowed = 405,
    kInternalServerError = 500
};

int Server::prefix_length(const std::string& req_location, const std::string& location)
{
    int len_min = std::min(req_location.size(), location.size());
    int len_match = 0;
    for (int i = 0; i < len_min; ++i) {
        if (req_location[i] != location[i])
            break;
        ++len_match;
    }
    return (len_match);
}

// for the moment it takes some static input
const Location* Server::routing(const std::string& req_location)
{
    int longest_match = 0;
    const Location* best_match = NULL;
    // iterrate through locations to find longest match
    for (size_t i = 0; i < config_->locations.size(); i++) {
        int len_match = prefix_length(req_location, config_->locations[i].path);
        if (len_match > longest_match) {
            best_match = &config_->locations[i];
        }
    }
    return (best_match ? best_match : &(config_->default_location));
}

int Server::valid_method(const std::string& method)
{
    for (size_t i = 0; i < config_->methods.size(); ++i) {
        if (method == config_->methods[i])
            return (0);
    }
    return (1);
}

// Description : build full path by concat 3 elements and consider for leading/trailing backslashes
// TO DO (DHE) : In the config parser -> ensure that root has no trailing /
std::string Server::build_request_path(const HttpRequest& request, const Location* best_match)
{
    std::string full_path = config_->root;
    std::string folder = static_cast<std::string>(best_match->path);
    // std::cout << "root = " << full_path << std::endl;
    std::string file =
        request.path.substr(best_match->path.size(), request.path.size() - best_match->path.size());
    if (full_path[full_path.size() - 1] != '/')
        full_path = full_path + "/";
    if (folder[0] == '/')
        folder.erase(0, 1);
    if (folder[folder.size() - 1] != '/')
        folder = folder + "/";
    if (file[0] == '/')
        file.erase(0, 1);
    full_path = full_path + folder + file;

    if (full_path[full_path.size() - 1] == '/')
        full_path = full_path + "index.html";
    return (full_path);
}

void Server::send_error(Client* conn, int error_code, std::string body)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << error_code << " " << reason_phrase(error_code) << "\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "\r\n"
        << body;
    std::string response = oss.str();
    send(conn->fd(), response.c_str(), response.size(),
         0); // bytes_sent can be used for debbuging
    std::cout << "fd = " << conn->fd() << " : " << error_code << " " << reason_phrase(error_code)
              << std::endl;
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, conn->fd(), NULL);
    remove_connection(conn);
}

void Server::handle_request(Client* conn, const HttpRequest& request)
{
    struct stat sb; // needed by stat to check if file exists
    if (valid_method(request.method) != 0) {
        send_error(conn, kMethodNotAllowed, ""); // in the end should be done
        return;
    }
    if (request.path.empty()) {
        send_error(conn, kBadRequest, "");
        return;
    }
    const Location* longest_match = routing(request.path);
    if (!longest_match) {
        send_error(conn, kNotFound, "");
        return;
    }
    std::cout << "longest_match = " << longest_match->path << std::endl;
    std::string full_path = build_request_path(request, longest_match);
    std::cout << "full_path = " << full_path << std::endl;

    if (request.method == "GET") {
        if (stat(full_path.c_str(), &sb) != 0) {
            send_error(conn, kNotFound, "");
        }
        else {
            send_response(conn); // here the file needs to be read and send to the socket
        }
        return;
    }
    else if (request.method == "DELETE") {
    }
    else if (request.method == "POST") {
    }
    else {
        send_error(conn, kMethodNotAllowed, "");
        return;
    }
    send_response(
        conn); // just for testing to avoid infinite loop as the socket remains ready for read/write
}

Server::Server(in_addr_t ip, in_port_t port, int limit_conn, const ServerConfig* config)
    : config_(config)
{
    fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd_ == -1) {
        throw SyscallError("socket", errno);
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = htonl(ip);

    if (::bind(fd_, (sockaddr*) &addr_, sizeof(addr_)) == -1) {
        throw SyscallError("bind", errno);
    }

    if (::listen(fd_, limit_conn) == -1) {
        throw SyscallError("listen", errno);
    }

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        throw SyscallError("epoll_create1", errno);
    }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd_, &ev) == -1) {
        throw SyscallError("epoll_ctl", errno);
    }

    list_head_ = NULL;
}

Server::~Server()
{
    while (list_head_) {
        Client* conn = list_head_;
        list_head_ = list_head_->next();
        delete conn;
    }
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
    if (fd_ != -1) {
        close(fd_);
    }
}

void Server::run()
{
    while (!g_sigint_received) {
        int n_events = epoll_wait(epoll_fd_, events_, WEBSERV_MAX_EVENTS, -1);
        if (n_events == -1) {
            switch (errno) {
            case EINTR:
                // epoll_wait was interrupted by a signal, just move on as if
                // nothing happened.
                n_events = 0;
                break;
            default:
                throw SyscallError("epoll_wait", errno);
            }
        }

        for (int i = 0; i < n_events; ++i) {
            Client* conn = (Client*) events_[i].data.ptr;

            // We set ev.data.ptr to NULL in the constructor, so we know for
            // sure this notification is coming from the server socket.
            if (conn == NULL) {
                accept_connection();
            }
            else {
                // reading from socket and storing request in Client (TO DO : IBOUKHSS)
                // for dev, mimic a http request and call the routing function
                struct HttpRequest request;
                request.method = "GET";
                request.path = "/files/42.txt";
                handle_request(conn,
                               request); // no try catch required except if we define a
                                         // scenario where we want to stop the server
            }
        }
    }
}

void Server::add_connection(Client* conn)
{
    conn->set_next(list_head_);
    if (list_head_) {
        list_head_->set_prev(conn);
    }
    list_head_ = conn;
}

void Server::remove_connection(Client* conn)
{
    Client* prev = conn->prev();
    Client* next = conn->next();

    if (prev) {
        prev->set_next(conn->next());
    }
    if (next) {
        next->set_prev(conn->prev());
    }
    if (conn == list_head_) {
        list_head_ = next;
    }

    delete conn;
}

// extracts the  first  connection  request  on  the  queue
// of pending connections for the listening socket
void Server::accept_connection()
{
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int client_fd = ::accept(fd_, (sockaddr*) &addr, &addr_len);
    if (client_fd == -1) {
        return;
    }

    Client* client = new Client(client_fd, addr);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = client;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
        throw SyscallError("epoll_ctl", errno);
    }

    add_connection(client);
}

void Server::send_response(Client* conn)
{
    int client_fd = conn->fd();
    HttpResponse res = {200, "text/plain", "Hello, client!\n"};
    std::string raw = res.to_string();

    send(client_fd, raw.data(), raw.size(), 0);

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
        throw SyscallError("epoll_ctl", errno);
    }

    remove_connection(conn);
}
