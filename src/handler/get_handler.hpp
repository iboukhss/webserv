#ifndef HANDLER_GET_HANDLER_HPP_
#define HANDLER_GET_HANDLER_HPP_

#include "Handler.hpp"
#include "core/client.hpp"
#include "src/core/client.hpp"
#include "sys/stat.h"

#include <string>

class GetHandler : public Handler {
public:
    explicit GetHandler(const Client* conn, const std::string& path);
    ~GetHandler();

    GetHandler(const GetHandler& other);
    GetHandler& operator=(const GetHandler& other);

    void GetHandler::onWritable(Client* conn);

private:
    GetHandler();
    int fd_;
    off_t size_;
    bool done_;
};

#endif
