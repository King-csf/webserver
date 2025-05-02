#include "timer.h"



timer_list::timer_list()
{
    head = tail = nullptr;
}

timer_list::~timer_list()
{
    Timer * temp = head;
    while(temp)
    {
        head = temp->next;
        delete temp;
        temp = head;
    }
    head = tail = nullptr;
}

void timer_list::addTimer(Timer* timer)
{
    if (!timer) {
        std::cout << "addTimer: timer is nullptr" << std::endl;
        return;
    }
    timer->prev = nullptr; 
    timer->next = nullptr;

    if (head == nullptr) { 
        head = tail = timer;
        return;
    }

    if (timer->expire < head->expire) { 
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }

    
    Timer* current = head;
    
    while (current->next != nullptr && timer->expire >= current->next->expire) {
        current = current->next;
    }

   
    timer->next = current->next;
    timer->prev = current;

    if (current->next != nullptr) 
    { 
        current->next->prev = timer; 
    } 
    else 
    { 
        tail = timer; 
    }
    current->next = timer; 
    
    
}

void timer_list::deleTimer(Timer * timer)
{
    if (!timer || !head) {
        std::cout << "deleTimer: timer or list is nullptr" << std::endl;
        return;
    }

    if (timer == head && timer == tail) 
    { 
        head = tail = nullptr;
    } 
    else if (timer == head) 
    { 
        head = head->next;
        if (head) 
        { 
            head->prev = nullptr;
        }
        
    } 
    else if (timer == tail) 
    { 
        tail = tail->prev;
        if (tail) 
        { 
            tail->next = nullptr;
        }
        
    } 
    else 
    { 
        if (timer->prev) {
            timer->prev->next = timer->next;
        }
        if (timer->next) {
            timer->next->prev = timer->prev;
        }
         
    }

    delete timer;
}


void timer_list::adjustTimer(Timer * timer)
{
    if (!timer || !head)
    {
        std::cout << "adjustTimer: timer or list is nullptr" << std::endl;
        return;
    }

    if (timer == head && timer == tail)
    {
        timer->expire = time(NULL) + 3 * 5;
        return;
    }

    if (timer == head)
    {
        head = timer->next;
        if (head)
        {
            head->prev = nullptr;
        }
    }
    else if (timer == tail)
    {
        tail = timer->prev;
        if (tail)
        {
            tail->next = nullptr;
        }
    }
    else
    {
        if (timer->prev)
        {
            timer->prev->next = timer->next;
        }
        if (timer->next)
        {
            timer->next->prev = timer->prev;
        }
    }

    timer->prev = nullptr;
    timer->next = nullptr;
    timer->expire = time(NULL) + 3 * 5;
    addTimer(timer);
}

void timer_list::tick()
{
    if (head == nullptr) {
        return; 
    }

    time_t current_time = time(NULL);
    Timer* temp = head;

    while (temp != nullptr && current_time >= temp->expire) {
        
        Timer* next_timer = temp->next;

        if (temp->cb_func) 
        {
            temp->cb_func(temp->info);                           
        }

        head = next_timer; 
        if (head) 
        {
            head->prev = nullptr;
        } else 
        {
            tail = nullptr; 
        }

        delete temp; 

        temp = head; 

    } 
}