#pragma once
#include <pthread.h>
//#include <bits/semaphore.h>
#include <semaphore.h>

class locker
{
public:
    locker()
    {
        pthread_mutex_init(&m_mutex,NULL);
    }

    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

class sem
{
public:
    sem()
    {
        sem_init(&m_sem,0,0);
    }
    
    
    sem(int num)
    {
        sem_init(&m_sem,0,num);
    }

    bool wait()
    {
        return sem_wait(&m_sem);
    }

    bool post()
    {
        return sem_post(&m_sem);
    }

    ~sem()
    {
        sem_destroy(&m_sem);
    }
private:
    sem_t m_sem;
};