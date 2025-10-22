

#include "Epoll.hpp"
#include "Server.hpp"

#include "unistd.h"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <vector>

#define MAX_EVENTS 10

int main(void)
{
    Server server(1000);
    int server_fd = server.getFd();
    Epoll epoll(server.getFd());
    int n_events;

    while (true) {
        // poll for available connections
        // two solutions possible : wait returns n events -> either loop 0 to n in the epoll_event
        // array held by the epoll object or modify the wait function to return an array
        n_events = epoll.wait();
        const epoll_event* events = epoll.getEvents();
        for (int i = 0; i < n_events; ++i) {
            if (events[i].data.fd == server_fd) {
                // we need to accept a new connection(s) -> there might be a queu
                while (1) {

                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(
                        server_fd, (struct sockaddr*)&client_addr,
                        &client_len); // extracts the  first  connection  request  on  the  queue
                                      // of pending connections for the listening socket
                    if (client_fd == -1) {
                        // error occured or no further connection request to be handled -> fine
                        // tuning later
                        break;
                    }
                    std::cout << "server event fd = " << events[i].data.fd
                              << "client_fd = " << client_fd << std::endl;
                    server.set_nonblocking(client_fd);
                    epoll.add(client_fd, EPOLLIN);
                }
            }
            else {
                // we need to write to a client socket
                const char* response = "HTTP/1.1 200 OK\r\n"
                                       "Content-Length: 14\r\n"
                                       "Content-Type: text/plain\r\n"
                                       "\r\n"
                                       "Hello, client!\n";
                send(events[i].data.fd, response, strlen(response), 0);
                epoll.remove(events[i].data.fd);
                close(events[i].data.fd);
            }
        }
    }
}
