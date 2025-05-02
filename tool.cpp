#include "tool.h"
#include "WebServer.h"



int *Tool::piped = 0;
int Tool::m_epolled = 0;

int Tool::setnoblock(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    fcntl(fd,F_SETFL,old_option | O_NONBLOCK);
    return old_option;
}

void Tool::addfd(int epolled,int fd,bool isOneShot)
{
    epoll_event events;
    events.data.fd = fd;
    events.events = EPOLLIN |EPOLLET | EPOLLRDHUP ;
    if(isOneShot)
    {
        events.events |= EPOLLONESHOT;
    }
    
    epoll_ctl(epolled,EPOLL_CTL_ADD,fd,&events);
    setnoblock(fd);
    
}

void Tool::deletefd(int epolled,int fd)
{
    epoll_ctl(epolled,EPOLL_CTL_DEL,fd,nullptr);
}

void Tool::addsig(int sig, void(*handler)(int))
{
    struct sigaction sa;
    memset(&sa,0,sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Tool::handler(int sig)
{
    int old_err = errno;
    int sig_info = sig;
    int ret = write(piped[1],(char*)&sig_info,1);
    if (ret == -1) {
        // 记录错误信息
        perror("send failed in Tool::handler");
    }
    //std::cout << "signal send success" << std::endl;
    errno = old_err;
}

void Tool::modifyfd(int fd,int op,bool isOneShot)
{
    struct epoll_event events;
    memset(&events, 0, sizeof(events)); // Properly initialize the events structure
    events.data.fd = fd;

    if(isOneShot)
    {
        events.events = op | EPOLLRDHUP | EPOLLONESHOT;
    }

    else
    {
        events.events = op | EPOLLRDHUP;
    }

    epoll_ctl(m_epolled,EPOLL_CTL_MOD,fd,&events);
}

string Tool::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;

    localtime_r(&now_time,&local_time);
    std::stringstream ss;
    ss << std::put_time(&local_time,"%Y-%m-%d %H:%M:%S");

    return ss.str();
}