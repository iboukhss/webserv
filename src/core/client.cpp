#include "core/client.hpp"

#include "core/virtual_server.hpp"
#include "http/http_parser.hpp"
#include "util/log_message.hpp"

#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

Client::Client(int fd, const VirtualServer& vs)
    : sockfd_(fd),
      vserv_(vs),
      state_(Client::kReceivingHeaders),
      handler_(NULL)
{
    epoll_fds_[sockfd_] = EPOLLIN | EPOLLRDHUP;
    pipefd_[0] = -1;
    pipefd_[1] = -1;
}

Client::~Client()
{
    if (sockfd_ != -1) {
        close(sockfd_);
    }
    if (handler_) {
        delete handler_;
    }
}

size_t Client::sendbuf_available_size() const
{
    return WEBSERV_MAX_SENDBUF_SIZE - sendbuf_.size();
}

void Client::want_read(int fd)
{
    epoll_fds_[fd] |= EPOLLIN;
}

void Client::want_write(int fd)
{
    epoll_fds_[fd] |= EPOLLOUT;
}

void Client::stop_read(int fd)
{
    epoll_fds_[fd] &= ~EPOLLIN;
}

void Client::stop_write(int fd)
{
    epoll_fds_[fd] &= ~EPOLLOUT;
}

void Client::on_epollin(int fd)
{
    if (fd == sockfd_) {
        read_from_socket();
        maybe_create_request_handler();
        maybe_write_to_regular_file();
    }
    else if (fd == pipefd_[0]) {
        read_from_pipe();
    }
    refresh_interest_list();
}

void Client::on_epollout(int fd)
{
    if (fd == sockfd_) {
        maybe_read_from_regular_file();
        write_to_socket();
    }
    else if (fd == pipefd_[1]) {
        write_to_pipe();
    }
    refresh_interest_list();
}

void Client::on_sync_complete()
{
    if (state_ == Client::kPreparingNextRequest) {
        parser_.reset_for_next_request();

        delete handler_;
        handler_ = NULL;

        epoll_fds_.clear();
        epoll_fds_[sockfd_] = EPOLLIN | EPOLLRDHUP;
        pipefd_[0] = -1;
        pipefd_[1] = -1;

        state_ = Client::kReceivingHeaders;
    }
}

// TODO: get rid of this
static void print_raw_data(const char* s, size_t n)
{
    std::cout << "< ";

    for (size_t i = 0; i < n; ++i) {
        char c = s[i];
        if (c == '\r') {
            std::cout << "\\r";
        }
        else if (c == '\n') {
            std::cout << "\\n\n";
            if (i < n - 1) {
                std::cout << "< ";
            }
        }
        else {
            std::cout.put(c);
        }
    }
    std::cout << "\n";
}

void Client::read_from_socket()
{
    if (state_ != Client::kReceivingHeaders && state_ != Client::kProcessingRequest)
        return;

    char buf[4096];

    while (!parser_.has_error() && !parser_.is_done() && parser_.available_size() > 0) {
        size_t max = std::min(parser_.available_size(), sizeof(buf));

        ssize_t n = recv(sockfd_, buf, max, 0);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                LOG(DEBUG) << "Client #" << sockfd_ << ": recv blocked, retrying";
                return;
            }
            LOG(ERROR) << "Client #" << sockfd_ << ": recv failed: " << strerror(errno);
            state_ = Client::kClosingConnection;
            return;
        }
        if (n == 0) {
            LOG(WARN) << "Client #" << sockfd_ << ": Connection closed by remote peer";
            state_ = Client::kClosingConnection;
            return;
        }

        parser_.append_data(buf, n);
        print_raw_data(buf, n);
    }

    if (parser_.has_error()) {
        LOG(ERROR) << "Client #" << sockfd_ << ": Invalid HTTP request";
        state_ = Client::kClosingConnection;
        return;
    }
}

void Client::write_to_socket()
{
    if (state_ != Client::kProcessingRequest)
        return;

    if (sendbuf_.empty())
        return;

    ssize_t n = send(sockfd_, sendbuf_.data(), sendbuf_.size(), 0);
    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG(WARN) << "Client #" << sockfd_ << ": send blocked, retrying";
            return;
        }
        LOG(ERROR) << "Client #" << sockfd_ << ": send failed: " << strerror(errno);
        state_ = Client::kClosingConnection;
        return;
    }
    if (n == 0) {
        LOG(WARN) << "Client #" << sockfd_ << "Failed to send any bytes!";
        return;
    }

    sendbuf_.erase(0, n);

    if (handler_->is_done() && sendbuf_.empty()) {
        if (keep_alive()) {
            state_ = Client::kPreparingNextRequest;
        }
        else {
            state_ = Client::kClosingConnection;
        }
    }
}

static void log_request_info(int fd, const HttpRequest& request)
{
    LOG(INFO) << "Client #" << fd << ": " << request.method << " " << request.path << " "
              << http_version_to_string(request.http_version);
}

void Client::maybe_create_request_handler()
{
    if (state_ != Client::kReceivingHeaders)
        return;

    if (!parser_.did_parse_headers())
        return;

    const HttpRequest& req = parser_.request();
    log_request_info(sockfd_, req);

    handler_ = vserv_.router().handle_request(req);
    pipefd_[0] = handler_->cgi_read_fd();
    pipefd_[1] = handler_->cgi_write_fd();

    if (pipefd_[0] != -1) {
        epoll_fds_[pipefd_[0]] = EPOLLIN;
    }
    if (pipefd_[1] != -1) {
        epoll_fds_[pipefd_[1]] = EPOLLOUT;
    }

    state_ = Client::kProcessingRequest;
}

void Client::maybe_write_to_regular_file()
{
    if (state_ == Client::kProcessingRequest) {
        assert(handler_ != NULL);

        if (handler_->is_regular_file()) {
            write_to_virtual_file();
        }
    }
}

void Client::maybe_read_from_regular_file()
{
    if (state_ == Client::kProcessingRequest) {
        assert(handler_ != NULL);

        if (handler_->is_regular_file()) {
            read_from_virtual_file();
        }
    }
}

void Client::read_from_pipe()
{
    assert(state_ == Client::kProcessingRequest);
    assert(handler_ != NULL);
    assert(handler_->cgi_read_fd() != -1);

    read_from_virtual_file();
}

void Client::write_to_pipe()
{
    assert(state_ == Client::kProcessingRequest);
    assert(handler_ != NULL);
    assert(handler_->cgi_write_fd() != -1);

    write_to_virtual_file();
}

void Client::read_from_virtual_file()
{
    LOG(DEBUG) << "Client::read_from_virtual_file()";
    LOG(DEBUG) << "!handler_->is_done() == " << !handler_->is_done();
    // LOG(DEBUG) << "handler_->has_output() == " << handler_->has_output();
    LOG(DEBUG) << "sendbuf_available_size() > 0 == " << (sendbuf_available_size() > 0);
    char buf[4096];
    // while (!handler_->is_done() && handler_->has_output() && sendbuf_available_size() > 0) {
    while (!handler_->is_done() && sendbuf_available_size() > 0) {
        size_t max = std::min(sizeof(buf), sendbuf_available_size());
        size_t n = handler_->read_output(buf, max);
        if (n == 0) {
            break;
        }
        sendbuf_.append(buf, n);
    }
}

void Client::write_to_virtual_file()
{
    LOG(DEBUG) << "Client::write_to_virtual_file()";
    LOG(DEBUG) << "!handler_->is_done() == " << !handler_->is_done();
    LOG(DEBUG) << "handler_->needs_input() == " << handler_->needs_input();
    LOG(DEBUG) << "parser_.has_body_chunk() == " << parser_.has_body_chunk();
    char buf[4096];
    while (!handler_->is_done() && handler_->needs_input() && parser_.has_body_chunk()) {
        size_t n = parser_.read_next_body_chunk(buf, sizeof(buf));
        size_t written = handler_->write_input(buf, n);
        parser_.consume_body_chunk(written);
        if (written < n) {
            break;
        }
    }
}

void Client::refresh_interest_list()
{
    for (std::map<int, uint32_t>::iterator it = epoll_fds_.begin(); it != epoll_fds_.end(); ++it) {
        it->second = 0;
    }

    if (state_ == Client::kClosingConnection) {
        return;
    }
    else if (state_ == Client::kReceivingHeaders) {
        want_read(sockfd_);
    }
    else if (state_ == Client::kProcessingRequest) {
        assert(handler_ != NULL);
        LOG(DEBUG) << "Client::kProcessingRequest";
        // IMPORTANT: refresh pipefd_[] from handler (it may have closed stdin)
        sync_pipe_fds_from_handler();
        if (handler_->needs_input()) { // socket
            want_read(sockfd_);
        }
        // if (handler_->has_output()) { // socket
        if (!sendbuf_.empty()) {
            LOG(DEBUG) << "EPOLLOUT on socket fd";
            want_write(sockfd_);
        }
        if (pipefd_[0] != -1 && !handler_->is_done()) {
            LOG(DEBUG) << "Client::kProcessingRequest => pipefd_[0] != -1 {want_read}";
            want_read(pipefd_[0]);
        }
        if (pipefd_[1] != -1 && handler_->needs_input()) {
            LOG(DEBUG) << "Client::kProcessingRequest => pipefd_[0] != -1 {want_write}";
            want_write(pipefd_[1]);
        }
    }
    else if (state_ == Client::kPreparingNextRequest) {
        want_read(sockfd_);
    }
    else {
        assert(0 && "UNREACHABLE");
    }
    epoll_fds_[sockfd_] |= EPOLLRDHUP;
}

void Client::sync_pipe_fds_from_handler()
{
    if (!handler_)
        return;

    int new_r = handler_->cgi_read_fd();
    int new_w = handler_->cgi_write_fd();

    // If handler closed stdout pipe (read end), remove old from interest list
    if (pipefd_[0] != -1 && new_r == -1) {
        epoll_fds_.erase(pipefd_[0]);
    }
    // If handler changed fd (rare but possible), update map key
    else if (pipefd_[0] != -1 && new_r != -1 && pipefd_[0] != new_r) {
        uint32_t mask = epoll_fds_[pipefd_[0]];
        epoll_fds_.erase(pipefd_[0]);
        epoll_fds_[new_r] = mask;
    }
    pipefd_[0] = new_r;

    // Same for stdin pipe (write end)
    if (pipefd_[1] != -1 && new_w == -1) {
        epoll_fds_.erase(pipefd_[1]);
    }
    else if (pipefd_[1] != -1 && new_w != -1 && pipefd_[1] != new_w) {
        uint32_t mask = epoll_fds_[pipefd_[1]];
        epoll_fds_.erase(pipefd_[1]);
        epoll_fds_[new_w] = mask;
    }
    pipefd_[1] = new_w;
}
