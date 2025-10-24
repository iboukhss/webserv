

#include "Epoll.hpp"

#include <stdlib.h>
#include <unistd.h>

#include <iostream>

void Epoll::add(int fd, uint32_t event)
{
    ev_.events = event;
    ev_.data.fd = fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev_) == -1) {
        close(fd); // to be confirmed that the client_fd needs to be closed here
        exit(EXIT_FAILURE);
    }
}

void Epoll::remove(int fd)
{
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev_) == -1) {
        // some message to log
        exit(EXIT_FAILURE);
    }
}

int Epoll::get_event_fd(int i)
{
    return (events_[i].data.fd);
}

epoll_event* Epoll::get_events()
{
    return (events_);
}

int Epoll::wait()
{
    int n = epoll_wait(epoll_fd_, events_, MAX_EVENTS, 100);
    if (n == -1) {
        exit(EXIT_FAILURE);
    }

    return (n);
}

Epoll::Epoll(int server_fd)
{
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        exit(EXIT_FAILURE);
    }
    ev_.events = EPOLLIN;    // we want to listen to -> file is available for read()
    ev_.data.fd = server_fd; // storing the server's fd in ev_
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd, &ev_) ==
        -1) { // add server_fd to interest list to monitor it
        // log error
        exit(EXIT_FAILURE);
    }
}

Epoll::~Epoll()
{
    std::cout << "Closing epoll_fd_" << std::endl;
    close(epoll_fd_);
}
