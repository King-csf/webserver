#pragma once


#include "http/http_connect.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cassert>
#include <sys/epoll.h>
#include <iostream>
#include <string.h>
#include <arpa/inet.h>
#include "./mmysql/Mysql.h"
#include "./threadpool/threadpool.hpp"
#include "tool.h"
#include "./timer/timer.h"

#define MAX_FD 65536
#define MIN_TIME 5


class webserver
{
public:
    webserver();
    ~webserver();

    int m_socked;
    int  m_epolled;
    int piped[2];
    epoll_event events[MAX_FD];
    http_conn * conn;

    //工具类
    Tool  tools;
    client_data *users;
    timer_list *tl;
    
    threadpool<http_conn> * pool;
    void threadpoolInit();

    //数据库连接池
    mysql_con_pool * con_pool;
    void mysqlInit();
    

    void dealClient();
    void eventListen();
    void dealSig();
    void eventLoop();
    void dealRead(int fd);
    void dealWrite(int fd);

    
    //计时器是否到时间
    bool timeover;
    //是否退出程序
    bool is_stop;
    void dealTimer();

};

//回调函数：删除文件描述符
void cb_func(client_data* data);
