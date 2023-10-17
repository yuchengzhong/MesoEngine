// Meso Engine 2024
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <boost/bind/bind.hpp>
using namespace boost::placeholders;
class ThreadPool 
{
private:
    boost::asio::io_service IOService;
    boost::shared_ptr<boost::asio::io_service::work> Work;
    boost::thread_group Threads;
    std::size_t Available = 0;
    std::size_t Size = 0;
    boost::mutex Mutex;

    std::map<boost::thread::id, uint32_t> ThreadIDs;
public:
    ThreadPool() : Work(new boost::asio::io_service::work(IOService)), Available(0){}
    uint32_t GetSize() const
    {
        return (uint32_t)Size;
    }
    //
    void Initialize(std::size_t Size_)
    {
        Size = Size_;
        Available = Size_;
        /*
        for (std::size_t i = 0; i < Size_; ++i)
        {
            Threads.create_thread(boost::bind(&boost::asio::io_service::run, &IOService));
        }
        */
        for (std::size_t i = 0; i < Size_; ++i)
        {
            Threads.create_thread([this, i]() 
            {
                ThreadIDs[boost::this_thread::get_id()] = i;
                IOService.run();
            });
        }
    }

    ~ThreadPool() 
    {
        Work.reset();
        Threads.join_all();
        IOService.stop();
    }

    template <typename Task, typename... Args>
    bool Enqueue(Task Task_, Args... Args_)
    {
        boost::unique_lock<boost::mutex> lock(Mutex);

        if (Available)
        {
            --Available;
            IOService.post(boost::bind(&ThreadPool::WrapTask<Task, Args...>, this, Task_, Args_...));//One bind
            return true;
        }
        else
        {
            return false;
        }
    }
    //Avoid double binding
    bool EnqueueForward(std::function<void()> TaskFunc)
    {
        boost::unique_lock<boost::mutex> lock(Mutex);

        if (Available)
        {
            --Available;
            IOService.post([this, TaskFunc]() 
                {
                    this->WrapTaskForward(TaskFunc);
                });
            return true;
        }
        else
        {
            return false;
        }
    }

    void WaitForTasksToComplete()
    {
        Work.reset();
        Threads.join_all();
        IOService.stop();
    }
    uint32_t GetCurrentThreadID()
    {
        return ThreadIDs[boost::this_thread::get_id()];
    }
private:
    template <typename Task, typename... Args>
    void WrapTask(Task Task_, Args... Args_)
    {
        try
        {
            Task_(Args_...);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
        }
        boost::unique_lock<boost::mutex> lock(Mutex);
        ++Available;
    }

    void WrapTaskForward(std::function<void()> TaskFunc)
    {
        try
        {
            TaskFunc();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in thread pool task: " << e.what() << std::endl;
        }
        boost::unique_lock<boost::mutex> lock(Mutex);
        ++Available;
    }
};