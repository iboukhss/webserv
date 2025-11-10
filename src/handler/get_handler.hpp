#ifndef HANDLER_GET_HANDLER_HPP_
#define HANDLER_GET_HANDLER_HPP_

#include "../core/client.hpp"
#include "handler.hpp"
#include "sys/stat.h"

#include <string>

class GetHandler : public Handler {
public:
    explicit GetHandler(Client* conn, const std::string& path);
    ~GetHandler();

    GetHandler(const GetHandler& other);
    GetHandler& operator=(const GetHandler& other);

    void on_writable(Client* conn);
    void write_headers(Client* conn);
    std::string derive_file_type();

private:
    GetHandler();
    int fd_;
    off_t size_;
    bool done_;
};

#endif
