#include "WebServer.h"
#include <sys/sendfile.h>


//int webserver::m_epolled = -1;

void webserver::mysqlInit()
{
    con_pool = mysql_con_pool::getIntance();
}
void webserver::threadpoolInit()
{
    pool = new threadpool<http_conn>();
    //pool->mysql_pool = mysql_con_pool::getIntance();
}

void cb_func(client_data * data)
{
    if(!data)
        return;
    epoll_ctl(Tool::m_epolled,EPOLL_CTL_DEL,data->fd,NULL);
    close(data->fd);
}

webserver::webserver()
{
    users = new client_data[MAX_FD];
    conn = new http_conn[MAX_FD];
    tl = new timer_list();
}
webserver::~webserver()
{
    delete tl;
    delete[] users;
}

void webserver::eventListen()
{
    m_socked = socket(AF_INET,SOCK_STREAM,0);
    if (m_socked == -1) {
        perror("socked error");
        exit(EXIT_FAILURE);
    }
    assert(m_socked != -1);
    int port = 9006;

    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_port = htons(port);
    ser_addr.sin_family = AF_INET;
    //绑定本机地址
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //端口复用
    int reuse = 1;
    setsockopt(m_socked,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    int ret = bind(m_socked,(sockaddr*)&ser_addr,sizeof(ser_addr));
    assert(ret != -1);

    ret = listen(m_socked,5);
    assert(ret != -1);

    ret = pipe(piped);
    if(ret == -1)
    {
        perror("piped init fail");
    }

    m_epolled = epoll_create(1);
    
    tools.addfd(m_epolled,m_socked,false);

    tools.setnoblock(piped[1]);
    tools.addfd(m_epolled,piped[0],false);
    tools.addsig(SIGALRM,tools.handler);
    tools.addsig(SIGTERM,tools.handler);

    alarm(MIN_TIME);
    
    
    Tool::m_epolled = m_epolled;
    Tool::piped = piped;

}

void webserver::eventLoop()
{
    timeover = false;
    is_stop = false;

    //epoll_event events[MAX_FD];
    while(true)
    {
        
        int num = epoll_wait(m_epolled,events,MAX_FD,-1);
        //std::cout << num << " ";
        for(int i = 0; i < num ; ++i)
        {
            if(events[i].data.fd == m_socked)
            {
                dealClient();
            }
            else if(events[i].events & EPOLLRDHUP)
            {
                std::cout << events->data.fd<<" disconnect" << std::endl;
                users[events[i].data.fd].cli_time->cb_func(&users[events[i].data.fd]);
                tl->deleTimer(users[events[i].data.fd].cli_time);
            }
            
            else if((events[i].data.fd == piped[0]) && (events[i].events & EPOLLIN))
            {
                //std::cout << "signal rec success" << std::endl;
                char buf[1024];
                int ret = read(events[i].data.fd,buf,1024);
                //std::cout << buf[0] << std::endl;
                for (int j = 0 ; j  < ret ; ++j)
                {
                    switch (buf[j])
                    {
                    case SIGALRM:
                        timeover = true;
                        //std::cout << "timerover change success" << std::endl;
                        break;
                    case SIGTERM:
                        is_stop = true;
                    default:
                        break;
                    }
                }
            }
            else if(events[i].events & EPOLLIN)
            {
                dealRead(events[i].data.fd);
            }
            else if(events[i].events & EPOLLOUT)
            {
                dealWrite(events[i].data.fd);
            }
        }
        if(timeover)
        {
            //std::cout << "timerover run" << std::endl;
            dealTimer();
        }
        else if(is_stop)
        {
            break;
        }
    }
}

//处理过期的定时器
void webserver::dealTimer()
{
    tl->tick();
    timeover = false;
    alarm(MIN_TIME);
    //std::cout << "run tick" << std::endl;
}

//处理客户端连接
void webserver::dealClient()
{
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    while(true)
    {
        
        int connd = accept(m_socked,(sockaddr*)&client_addr,&len);
        //std::cout << connd << " ";
        if (connd < 0) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
              
              break;
            }
            if (errno == EINTR) continue; 
            
            perror("accept failed");  
            break;
          }
        if(connd >= MAX_FD)
        {
            close(connd);
            perror("connd > MAX_FD");
            return;
        }
        tools.addfd(m_epolled,connd,true);
        
        Timer * timer = new Timer();
        timer->cb_func = cb_func;
        timer->expire = time(NULL) + 3 * MIN_TIME;
        
        char ip[50];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        //std::cout << ip << " connect" << std::endl;
        users[connd].client_addr = client_addr;
        users[connd].fd = connd;
        users[connd].cli_time = timer;
        
        timer->info = &users[connd];

        tl->addTimer(timer);

        //conn[fd]
        

    }
}

void webserver::dealRead(int fd)
{
   conn[fd].fd = fd;
   conn[fd].addr = users[fd].client_addr;
   //conn[fd].readOnce();
   
   
   if(users[fd].cli_time)
   {
        tl->adjustTimer(users[fd].cli_time);
   }
   
   pool->addTask(&conn[fd],0);

   while(true)
   {
        if(conn[fd].improv )
        {
            if(conn[fd].interrupt)
            {
                users[fd].cli_time->cb_func(&users[fd]);
                tl->deleTimer(users[fd].cli_time);
                conn[fd].interrupt = 0;
            }
            conn[fd].improv = 0;
            break;
        }
   }

   /*conn[fd].mysql = con_pool->getConnect();
   conn[fd].test();
  
   con_pool->releaseConnect(conn[fd].mysql);
   tools.modifyfd(fd,EPOLLOUT,true);*/
   
}

void webserver::dealWrite(int fd)
{
    
    //tl->adjustTimer(users[fd].cli_time);
    //conn[fd].process();

    if (users[fd].cli_time)
        {
            tl->adjustTimer(users[fd].cli_time);
        }

        pool->addTask(&conn[fd],1);

        while (true)
        {
            if (conn[fd].improv)
            {
                if (conn[fd].interrupt)
                {
                    users[fd].cli_time->cb_func(&users[fd]);
                    tl->deleTimer(users[fd].cli_time);
                    conn[fd].interrupt = 0;
                }
                conn[fd].improv = 0;
                break;
            }
        }
    
}