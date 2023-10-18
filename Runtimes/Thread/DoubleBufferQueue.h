#include <queue>
#include <atomic>

template <typename T>
class TDoubleBufferQueue
{
private:
    using FQueue = std::queue<T>;
    FQueue Queues[2];
    FQueue* WriteQueue;
    FQueue* ReadQueue;
    mutable std::mutex QueueMutex; // For push and swap

public:
    TDoubleBufferQueue() : WriteQueue(&Queues[0]), ReadQueue(&Queues[1])
    {
    }

    void Push(const T& Item)
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        WriteQueue->push(Item);
    }
    void Push(T&& Item)
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        WriteQueue->push(std::move(Item));
    }

    void Swap()
    {
        std::lock_guard<std::mutex> Lock(QueueMutex);
        std::swap(WriteQueue, ReadQueue);
    }

    bool Pop(T& Item)
    {
        if (ReadQueue->empty())
        {
            return false;
        }

        Item = std::move(ReadQueue->front());
        ReadQueue->pop();
        return true;
    }

    size_t Size() const
    {
        return ReadQueue->size();
    }
};