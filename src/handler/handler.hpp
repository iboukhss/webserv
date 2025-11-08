#ifndef HANDLER_HANDLER_HPP_
#define HANDLER_HANDLER_HPP_

#include "src/core/client.hpp"

#include <string>

class Handler {
public:
    explicit Handler(const std::string& path);
    virtual ~Handler();

    virtual void onWritable() = 0;
    bool isDone();

protected:
    const std::string path_;

private:
    Handler();
    bool done_;
};

#endif
