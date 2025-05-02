#pragma once
#include <fcntl.h>
#include <sys/epoll.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <string>

//#include "WebServer.h"
#include "timer/timer.h"
//class webserver;

using std::string;

class Tool
{
public:
    static int m_epolled;
    static int *piped;

    int setnoblock(int fd);
    void addfd(int epolled,int fd,bool isOneShot);
    void modifyfd(int fd,int op,bool isOneShot);
    void deletefd(int epolled,int fd);
    void addsig(int sig,void (*handler)(int));
    //信号处理函数
    static void handler (int sig);

    //获取当前时间
    string getCurrentTime();
};

