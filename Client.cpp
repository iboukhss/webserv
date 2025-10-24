

#include "Client.hpp"

Client::Client(Socket* socket)
    : socket_(socket)
{
}

Client::~Client()
{
    delete socket_;
}
