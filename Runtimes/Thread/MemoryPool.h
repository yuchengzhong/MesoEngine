// Meso Engine 2024
#pragma once
#include <iostream>
#include <map>
#include <mutex>

template<typename T>
class TMemoryPool
{
public:
    std::vector<T> Pool;
    uint32_t CurrentIndex;
    uint32_t Size = 0;

    //TODO: mutex
public:
    TMemoryPool()
    {

    }
    TMemoryPool(uint32_t Size_)
        : Pool(Size_), CurrentIndex(0), Size(Size_)
    {}
    void Initialize(uint32_t Size_)
    {
        Pool.resize(Size_);
        CurrentIndex = 0;
        Size = Size_;
    }
    T& GetCurrent()
    {
        return Pool[CurrentIndex];
    }
    const T& GetCurrent() const
    {
        return Pool[CurrentIndex];
    }
    void Next()
    {
        CurrentIndex = (CurrentIndex + 1) % Size;
    }
    T& operator[](uint32_t Index)
    {
#if not defined(NDEBUG)
        assert(Index < Size && "Index out of range");
#endif
        return Pool[Index];
    }

    const T& operator[](uint32_t Index) const
    {
#if not defined(NDEBUG)
        assert(Index < Size && "Index out of range");
#endif
        return Pool[Index];
    }
};