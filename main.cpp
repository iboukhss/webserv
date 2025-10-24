

#include "Epoll.hpp"
#include "Server.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>

#define MAX_EVENTS 10

int main(void)
{
    Server server(INADDR_ANY, 8080, 1000); // later params retrieved from config file
    int server_fd = server.listen_fd();
    Epoll epoll(server_fd);

    while (true) {
        // poll for available connections
        // two solutions possible : wait returns n events -> either loop 0 to n in the epoll_event
        // array held by the epoll object or modify the wait function to return an array
        int n_events = epoll.wait();

        for (int i = 0; i < n_events; ++i) {
            int event_fd = epoll.get_event_fd(i);
            if (event_fd == server_fd) {

                // we need to accept a new connection(s) -> there might be a queue
                while (true) {
                    int client_fd = server.accept_conn();
                    if (client_fd == -1) {
                        break;
                    }

                    std::cout << "server event fd = " << event_fd << "client_fd = " << client_fd
                              << std::endl;
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
                server.send_response(event_fd, response);
                epoll.remove(event_fd);
                close(event_fd); // not needed as by removing the event_fd from the interest list,
                                 // the fd is closed ?
            }
        }
    }
}
