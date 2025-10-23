

// singleton ?

#ifndef SERVER_H_
#define SERVER_H_

#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

class Server {
public:
    Server(int port, int limit_conn);
    ~Server();

    int set_nonblocking(int fd);
    int getFd();
    int acceptConn();
    void sendResponse(int event_fd, const char* response);

private:
    Server();
    int server_fd_;
    struct sockaddr_in serverAddress_;
    std::vector<int> conn;
    // vector clients connections
};

#endif
