// Meso Engine 2024
#pragma once
#include <iostream>
#include <map>
#include <mutex>

template<typename K, typename V, typename Comp = std::less<K>>
class TThreadSafeMap 
{
private:
    using MapType = std::map<K, V, Comp>;
    using LockType = std::lock_guard<std::mutex>;
    MapType map_;
    mutable std::mutex mutex_;

public:
    void ATOMIC_insert(const K& key, const V& value)
    {
        LockType lock(mutex_);
        map_[key] = value;
    }

    bool ATOMIC_not_contains_insert(const K& key, const V& value, V& original_value) //return true if not contains, then add. if contains, return false, and do nothing
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        bool contains = it != map_.end();
        if (!contains)
        {
            map_[key] = value;
            return true;
        }
        else
        {
            if (it != map_.end())
            {
                original_value = it->second;
                return false;
            }
        }
    }

    bool ATOMIC_get(const K& key, V& value) const
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) 
        {
            value = it->second;
            return true;
        }
        return false;
    }

    bool ATOMIC_remove(const K& key)
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) 
        {
            map_.erase(it);
            return true;
        }
        return false;
    }
    //Takes in <V>(V)
    template<typename Functor>
    bool ATOMIC_modify(const K& key, Functor fn)
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            map_.insert(key, fn(it));
            return true;
        }
        return false;
    }
    //Takes in <bool>(V)
    template<typename Functor>
    bool ATOMIC_remove_by_condition(const K& key, Functor fn)
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end() && fn(it->second))
        {
            map_.erase(it);
            return true;
        }
        return false;
    }
    template<typename Functor>
    bool ATOMIC_remove_insert_by_condition(const K& key, const K& key2, const V& value, Functor fn)
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end())
        {
            if (fn(it->second))
            {
                map_.erase(it);
            }
            else
            {
                return false;
            }
        }
        map_[key2] = value;
        return true;
    }
    bool ATOMIC_remove_and_insert(const K& key, const K& key2, const V& value)
    {
        LockType lock(mutex_);
        auto it = map_.find(key);
        if (it != map_.end()) 
        {
            map_.erase(it);
            map_[key2] = value;
            return true;
        }
        map_[key2] = value;
        return false;
    }

    size_t ATOMIC_size() const
    {
        LockType lock(mutex_);
        return map_.size();
    }

    bool ATOMIC_contains(const K& key) const
    {
        LockType lock(mutex_);
        return map_.find(key) != map_.end();
    }

    void ATOMIC_clear()
    {
        LockType lock(mutex_);
        map_.clear();
    }

    MapType ATOMIC_get_copy() const
    {
        LockType lock(mutex_);
        return map_;
    }
};
