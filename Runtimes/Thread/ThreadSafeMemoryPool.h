#pragma once
#include <iostream>
#include <map>
#include <mutex>
#include "AtomicVector.h"

template<typename T>
class TThreadSafeMemoryPool
{
public:
    std::vector<T> Pool;
    TAtomicVector<uint64_t> CurrentIndex;
    std::atomic<uint32_t> RunningThreadCount = 0;
    std::vector<std::unique_ptr<std::mutex>> Locks;
    uint32_t Size = 0;
    uint32_t ThreadCount = 1;
    uint32_t ThreadPartition = 1;

    std::atomic<bool> bSyncing = false;
public:
    TThreadSafeMemoryPool()
    {

    }
    TThreadSafeMemoryPool(uint32_t Size_, uint32_t ThreadCount_ = 1)
        : Pool(Size_), CurrentIndex(ThreadCount_), Locks(Size_), Size(Size_), ThreadCount(ThreadCount_)
    {}
    void Initialize(uint32_t Size_, uint32_t ThreadCount_ = 1)
    {
        ThreadCount_ = std::max(1u, ThreadCount_);
        Pool.resize(Size_);
        CurrentIndex.Initialize(ThreadCount_);
        Locks.resize(Size_);
        for (auto& LockPtr : Locks) 
        {
            LockPtr = std::make_unique<std::mutex>();
        }
        Size = Size_;
        ThreadCount = ThreadCount_;
        ThreadPartition = std::max((Size / ThreadCount), 1u);
    }
    bool Acquire(uint32_t& ValidIndex, uint32_t ThreadIndex = 0)
    {
        uint32_t StartIndex = ThreadPartition * ThreadIndex;
        uint32_t EndIndex = (ThreadIndex == ThreadCount - 1) ? Size : ThreadPartition * (ThreadIndex + 1);
        //printf("Thread id %d, %d-%d\n", ThreadIndex, StartIndex, EndIndex);
        for (uint32_t i = StartIndex; i < EndIndex; ++i)
        {
            uint32_t Index = StartIndex + uint32_t(CurrentIndex[ThreadIndex]++ % uint64_t(EndIndex - StartIndex));
            if (Locks[Index]->try_lock())
            {
                ValidIndex = Index;
                RunningThreadCount++;
                return true;
            }
        }
        return false; // All elements are locked
    }
    void Release(const uint32_t Index)
    {
        Locks[Index]->unlock();
        RunningThreadCount--;
    }
    T& operator[](uint32_t Index)
    {
        if (Index >= Size)
        {
            throw std::out_of_range("Index out of range");
        }
        return Pool[Index];
    }

    const T& operator[](uint32_t Index) const
    {
        if (Index >= Size)
        {
            throw std::out_of_range("Index out of range");
        }
        return Pool[Index];
    }
    template<typename ReturnType>
    std::vector<ReturnType> GetValidItem(std::function<ReturnType(const T&)> ConstructorFunction)
    {
        std::vector<ReturnType> result;
        for (uint32_t i = 0; i < Size; i++) 
        {
            if (Locks[i]->try_lock())
            {
                std::lock_guard<std::mutex> Guard(*Locks[i].get(), std::adopt_lock);
                if (Pool[i].bIsvalid())
                {
                    result.push_back(ConstructorFunction(Pool[i]));
                }
            }
        }
        return result;
    }
};