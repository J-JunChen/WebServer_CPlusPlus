#include "epoller.h"

Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)),
events_(maxEvent){
    /* epoll_create(size) size 参数不起作用，只是给内核一个提示，告诉它事件表需要多大。
    */
    assert(epollFd_ >= 0 && events_.size() > 0);
}

Epoller::~Epoller()
{
}


bool Epoller::AddFd(int fd, uint32_t events){
    int(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    /*
        9.3.1 epoll_ctl 操作 epoll 的内核事件表
        EPOLL_CTL_ADD：添加 fd
        EPOLL_CTL_DEL: 删除 fd 上的注册事件。
        EPOLL_CTL_MOD: Change the event event associated with the target file descriptor fd.

        int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);

        Description
        This system call performs control operations on the epoll instance referred to by the file descriptor 'epfd'. It requests that the operation 'op' be performed for the target file descriptor, 'fd'.
    */
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
    /*
        Register the target file descriptor fd on the epoll instance referred to by the file descriptor 'epfd' and associate the event 'event' with the internal file linked to 'fd'.
    */
}

bool Epoller::DelFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
    /*
        Remove (deregister) the target file descriptor 'fd' from the epoll instance referred to by 'epfd'. The 'event' is ignored and can be NULL (but see BUGS below).
    */
}

int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
    /*
int epoll_wait(int epfd, struct epoll_event *events,
                      int maxevents, int timeout);
        The epoll_wait() system call waits for events on the epoll instance referred to by the file descriptor 'epfd'.  The buffer pointed to by 'events' is used to return information from the ready list about file descriptors in the interest list that have some events available.
    */
}

int Epoller::GetEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}