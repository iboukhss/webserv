

#include "Client.hpp"

#include <cstddef>

Client::Client(Socket* socket)
    : socket_(socket),
      next_(NULL),
      prev_(NULL)
{
}

Client::~Client()
{
    delete socket_;
}
