#include "core/client.hpp"

#include "util/syscall_error.hpp"

#include <fcntl.h>
#include <unistd.h>

void Client::set_handler(Handler* handler)
{
    handler_ = handler;
}

// send buffer, if buffer empty then cal
void Client::on_writable()
{
    if (handler_ && !handler_->is_done())
        handler_->on_writable(this);
}

bool Client::handler_is_done()
{
    return (handler_->is_done());
}

void Client::flush_send_bufffer()
{
    if (send_buffer_.empty())
        return;
    // sending buffer
    ssize_t n = send(fd_, send_buffer_.c_str(), send_buffer_.size(), 0);
    // erase buffer
    if (n > 0) {
        send_buffer_.erase(0, send_buffer_.size());
    }
    else if (n == 0) {
        // mark the client as closed
    }
}

Client::Client(int fd, const sockaddr_in& addr)
    : fd_(fd),
      addr_(addr),
      prev_(NULL),
      next_(NULL),
      handler_(NULL)

{
    set_nonblocking();
}

Client::~Client()
{
    if (fd_ != -1) {
        close(fd_);
    }
    if (handler_) {
        delete (handler_);
    }
}

void Client::set_nonblocking()
{
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
        throw UnrecoverableError("fcntl", errno);
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw UnrecoverableError("fcntl", errno);
    }
}
