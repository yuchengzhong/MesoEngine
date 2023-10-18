#include <mutex>
#include <queue>

template<typename T>
class TThreadSafeQueue 
{
private:
    std::queue<T> Queue;
    std::mutex Lock;

public:
    void AssertNotEmpty() const
    {
#if not defined(NDEBUG)
        assert(!Queue.empty() && "Index out of empty!");
#endif
    }

    void Push(const T& Value) 
    {
        std::lock_guard<std::mutex> Lock_(Lock);
        Queue.push(Value);
    }
    
    void Push(T&& Value) 
    {
        std::lock_guard<std::mutex> Lock_(Lock);
        Queue.push(std::move(Value));
    }

    bool Pop(T& OutValue) 
    {
        std::lock_guard<std::mutex> Lock_(Lock);
        if (Queue.empty())
        {
            return false;
        }
        OutValue = Queue.front();
        Queue.pop();
        return true;
    }

    size_t Size()
    {
        std::lock_guard<std::mutex> Lock_(Lock);
        return Queue.size();
    }

    T& Front()
    {
        std::lock_guard<std::mutex> Lock_(Lock);
        AssertNotEmpty();
        return Queue.front();
    }
};