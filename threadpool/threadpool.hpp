#include <pthread.h>
#include <queue>
#include <vector>
#include "../locker/locker.h"
#include "../mmysql/Mysql.h"
#include "../http/http_connect.h"

using std::queue;
using std::vector;


template<class T>
class threadpool
{
public:
    threadpool();
    ~threadpool();

    int max_thread;
    int have_use_thread;
    int free_thread;

    queue<T*> task;
    vector<pthread_t> thread_list;
    mysql_con_pool * mysql_pool;

    static void* work(void*arg);
    void run();

    void addTask(T* quest,int flag);
    T* popTask();

    locker lock;
    sem m_sem;
};

template<class T>
threadpool<T>::threadpool()
{
    mysql_pool = mysql_con_pool::getIntance();

    max_thread = 15;
    have_use_thread = 0;
    free_thread = max_thread;

    for(int i = 1; i <= max_thread ;++i)
    {
        pthread_t t;
        int rc = pthread_create(&t, NULL, work, this);
        if (rc != 0)
        {
            perror("pthread_create failed");
        }
        else
        {
            pthread_detach(t);
            thread_list.push_back(t);
        }
    }

    m_sem = sem(0);
}

template<class T>
threadpool<T>::~threadpool()
{
    task.clear();
    thread_list.clear();
}


template<class T>
void threadpool<T>::addTask(T* quest,int flag)
{
    if(!quest)
    {
        return;
    }
    lock.lock();
    quest->flag = flag;
    task.push(quest);

    lock.unlock();

    m_sem.post();

}

template<class T>
T* threadpool<T>::popTask()
{
    m_sem.wait(); 

    lock.lock();
    T* temp = task.front();
    task.pop();

    lock.unlock();

    return temp;
}

template<class T>
void* threadpool<T>::work(void*arg)
{
    threadpool<T> * pool = (threadpool<T>*)arg;
    pool->run();
    return pool;
}

template<class T>
void threadpool<T>::run()
{
    while(true)
    {
        //cout << getpid()
        T*  quest = popTask();
        if(!quest)
        {
            continue;
        }

        
        //处理读
        if(quest->flag == 0)
        {
            if (quest->readOnce())
            {
                
                quest->mysql = mysql_pool->getConnect();
                
                quest->test();

                
                mysql_pool->releaseConnect(quest->mysql);
                quest->mysql = nullptr;
                
                quest->improv = 1;
            }
            else 
            {
                quest->improv = 1;
                quest->interrupt = 1;
            }
        }
        //发送数据
        else 
        {
           if( quest->process())
           {
                quest->improv = 1;
           }
           else
           {
                quest->improv = 1;
                quest->interrupt = 1;
           }
           
           
        }
    }
}