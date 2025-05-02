#pragma once
#include <mysql/mysql.h>
#include "../locker/locker.h"
#include<string>
#include<list>
using std::string;
using std::list;

class mysql_con_pool
{
public:
    locker lock;
    sem m_sem;

    string ip;
    int port;
    string user_name;
    string db_name;
    string passward;
    

    int max_con;
    int have_con;
    int free_con;

    list<MYSQL*> con_list;

    mysql_con_pool();
    ~mysql_con_pool();

    //获取连接
    MYSQL* getConnect();
    //释放连接
    void releaseConnect(MYSQL * mysql);

    static mysql_con_pool* getIntance();

    void init(string ip,int port,string user_name,string db_name,string passward);
    void destory();
};