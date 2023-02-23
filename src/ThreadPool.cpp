#include"ThreadPool.h"
#include<thread>
#include<fstream>
#include<iostream>
#include"Config.h"
Config Config::instance{"config.json"};
ThreadPool ThreadPool::instance;
extern unsigned long long success_nums;
extern unsigned long long nums_2;
extern unsigned long long nums_3;
extern unsigned long long nums_4;
ThreadPool::ThreadPool():loop_thread{0},current_task{0}
{
    auto& config=Config::getInstance();
    max_thread=config.thread_nums;
    min_task=config.min_tasks;
    max_task=config.max_tasks;
    ic=new boost::asio::io_context[max_thread];
    for(unsigned i=0;i<max_thread;i++)
    {
        std::thread t{&ThreadPool::run,this,i};
        t.detach();
    }
    std::thread t{
        [this]
        {
            boost::asio::io_context::work work{file_ic};
            file_ic.run();
        }
    };
    t.detach();
}

ThreadPool::~ThreadPool()
{
    delete[] ic;
}

void ThreadPool::run(unsigned i)
{
    boost::asio::io_context::work work{ic[i]};
    ic[i].run();
}

ThreadPool& ThreadPool::getInstance()
{
    return instance;
}

boost::asio::io_context& ThreadPool::getIocontext()
{
    m1.lock();
    current_task++;
    m1.unlock();
    std::unique_lock ul{m2};
    if(current_task>max_task)
        cv.wait(ul);

    loop_thread++;
    return ic[loop_thread%max_thread];
}

void ThreadPool::CompleteTask(int _success)
{
    m1.lock();
    current_task--;
    m1.unlock();


    if(current_task<min_task)
        cv.notify_all();

    if(_success==-1)return;
    m3.lock();
    success_nums++;
    if(_success==2)nums_2++;
    else if(_success==3)nums_3++;
    else if(_success==4)nums_4++;
    m3.unlock();
}

boost::asio::io_context& ThreadPool::getLogIC()
{
    return file_ic;
}

void ThreadPool::zero_nums()
{
    m3.lock();
    success_nums=0;
    nums_2=0;
    nums_3=0;
    nums_4=0;
    m3.unlock();
}
