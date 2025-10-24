#include "Server.hpp"

#include <stdlib.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

Server::Server(in_addr_t ip, uint16_t port)
    : socket_(AF_INET, SOCK_STREAM, 0)
{
    sockaddr_in addr;

    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = ip;
    socket_.bind(addr);
    socket_.listen(5);

    std::cout << "Server listening on port " << port << "...\n";
}

Server::~Server()
{
    for (size_t i = 0; i < clients_.size(); ++i) {
        delete clients_[i];
    }
}

void Server::accept_client()
{
    Socket* socket = socket_.accept();
    Client* client = new Client(socket);
    clients_.push_back(client);
}

void Server::run()
{
    while (1) {
        accept_client();
    }
}
