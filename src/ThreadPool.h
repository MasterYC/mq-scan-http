#pragma once
#include<boost/asio/io_context.hpp>
#include<mutex>
#include<condition_variable>
class ThreadPool
{
public:
    ThreadPool(const ThreadPool&)=delete;
    ThreadPool(ThreadPool&&)=delete;
    ThreadPool& operator=(const ThreadPool&)=delete;
    ThreadPool& operator=(ThreadPool&&)=delete;

    static ThreadPool& getInstance();
    boost::asio::io_context& getIocontext();
    boost::asio::io_context& getLogIC();
    void CompleteTask(int _success);
    void zero_nums();
private:
    unsigned min_task;
    unsigned max_task;
    std::mutex m1;
    std::mutex m2;
    std::mutex m3;
    std::condition_variable cv;
    boost::asio::io_context* ic;
    boost::asio::io_context file_ic;
    unsigned current_task;
    unsigned max_thread;
    unsigned loop_thread;
    static ThreadPool instance;
    void run(unsigned i);
    ThreadPool();
	~ThreadPool();
};