#pragma once
#include <vector>
#include <atomic>
#include <memory>

template<typename T>
class TAtomicVector 
{
private:
    std::vector<std::unique_ptr<std::atomic<T>>> Data;

    void AtomicInitializeAtomicArray(uint32_t ArraySize, const T& InitializedValue) 
    {
        Data.resize(ArraySize);
        for (auto& p : Data)
        {
            p = std::make_unique<std::atomic<T>>(InitializedValue);
        }
    }

public:
    TAtomicVector()
    {

    }
    void Initialize(uint32_t Size_, const T& InitialValue = T())
    {
        AtomicInitializeAtomicArray(Size_, InitialValue);
    }
    void Initialize(uint32_t Size_, T InitialValue)
    {
        AtomicInitializeAtomicArray(Size_, InitialValue);
    }
    explicit TAtomicVector(size_t Size_, const T& InitialValue = T())
    {
        AtomicInitializeAtomicArray(Size_, InitialValue);
    }

    void Set(size_t Index, const T& Value) 
    {
        Data[Index]->store(Value);
    }

    T Get(size_t Index) const
    {
        return Data[Index]->load();
    }

    void Increment(size_t Index)
    {
        Data[Index]->fetch_add(1);
    }

    std::vector<std::unique_ptr<std::atomic<T>>>& NonAtomicAccess() 
    {
        return Data;
    }

    std::atomic<T>& operator[](size_t Index) 
    {
        return *Data[Index].get();
    }

    const std::atomic<T>& operator[](size_t Index) const 
    {
        return *Data[Index].get();
    }
};