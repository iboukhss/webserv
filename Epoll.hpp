

/*to clarify :
    - singleton ?
    - edge-triggered vs level-triggered ?


    struct epoll_event {
    uint32_t     events;    //Epoll events -> like EPOLLIN, EPOLLOUT
    epoll_data_t data;      //allows to store anything -> event.data.fd = target_fd
    };


*/

#ifndef EPOLL_H_
#define EPOLL_H_

#include "sys/epoll.h"
#include <iostream>

#define MAX_EVENTS 10 // MAX EVENTS THAT CAN BE LOGGED BEFORE WAIT IS RETURNING

class Epoll {
public:
    Epoll(int server_fd);
    ~Epoll();

    void add(int fd, uint32_t event);
    void remove(int fd);
    epoll_event* getEvents();
    int wait();

private:
    Epoll();
    int epoll_fd_;          // fd of the epoll instance
    struct epoll_event ev_; // holds the event types we are listening for as well as server_fd
    struct epoll_event events_[MAX_EVENTS]; // buffer for events "detected" by epoll
};

#endif
