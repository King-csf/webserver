#include "Mysql.h"

mysql_con_pool::mysql_con_pool()
{
    

    init("192.168.111.200",3306,"cui","test","123456");
}

mysql_con_pool::~mysql_con_pool()
{
    
    destory();
}


mysql_con_pool* mysql_con_pool::getIntance()
{
    static mysql_con_pool pool;
    return &pool;
}


void mysql_con_pool::init(string ip,int port,string user_name,string db_name,string passward)
{
    //lock.lock();
    max_con = 20;
    have_con = 0;

    for(int i = 1; i <= max_con;++i)
    {
        MYSQL * mysql = mysql_init(NULL);
        if(mysql == NULL)
        {
            perror("mysql init error");
            continue;
        }
        
        if(mysql_real_connect(mysql,ip.c_str(),user_name.c_str(),passward.c_str(),db_name.c_str(),port,NULL,0) == NULL)
        {
            perror("mysql connect error");
            continue;

        }

        con_list.push_back(mysql);

        free_con++;
    }
    
    m_sem = sem(free_con);
    //lock.unlock();

}

MYSQL * mysql_con_pool::getConnect()
{
    m_sem.wait();

    lock.lock();

    MYSQL* mysql = con_list.front();
    con_list.pop_front();

    have_con++;
    free_con--;

    lock.unlock();
    return mysql;

}

void mysql_con_pool::releaseConnect(MYSQL * mysql)
{
    lock.lock();

    MYSQL* temp = mysql;
    con_list.push_back(temp);

    have_con--;
    free_con++;

    lock.unlock();

    m_sem.post();
}

void mysql_con_pool::destory()
{
    lock.lock();

    for(auto it = con_list.begin(); it != con_list.end(); ++it )
    {
        MYSQL * mysql = *it;
        mysql_close(mysql);
    }

    max_con = 0;
    have_con = 0;
    
    con_list.clear();

    lock.unlock();
}