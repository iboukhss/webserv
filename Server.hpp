

// singleton ?

#ifndef SERVER_H_
#define SERVER_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>

class Server {
public:
    Server(int limit_conn);
    ~Server();

    int set_nonblocking(int fd);
    int getFd();

private:
    Server();
    int server_fd_;
    struct sockaddr_in serverAddress_;
    std::vector<int> conn;
    // vector clients connections
};

#endif
