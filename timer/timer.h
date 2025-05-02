#pragma once

#include <netinet/in.h>
#include <cstddef>
#include <sys/epoll.h>
//#include "../WebServer.h"

#include <unistd.h>
#include <iostream>

class client_data;

 
class Timer
{
public:
        Timer():prev(NULL),next(NULL){};
    ~Timer(){};

    client_data * info;  
    Timer * prev;
    Timer * next;
    time_t expire;
    void (*cb_func)(client_data*);
};

    

class client_data
{
public:
    sockaddr_in client_addr;
    int fd;
    Timer *cli_time;
};


//定时器队列
class timer_list
{
public:
    timer_list();
    ~timer_list();

    Timer * head;
    Timer * tail;

    void addTimer(Timer * node);
    void deleTimer(Timer * node);
    void adjustTimer(Timer * nide);
    void tick();

};

