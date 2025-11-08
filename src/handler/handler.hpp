#ifndef HANDLER_HANDLER_HPP_
#define HANDLER_HANDLER_HPP_

#include "../core/client.hpp"

#include <string>

class Handler {
public:
    explicit Handler(const std::string& path);
    virtual ~Handler();

    Handler(const Handler& other);
    Handler& operator=(const Handler& other);

    virtual void on_writable(Client* conn) = 0;
    bool is_done();

protected:
    const std::string kPath;

private:
    Handler();
    bool done_;
};

#endif
