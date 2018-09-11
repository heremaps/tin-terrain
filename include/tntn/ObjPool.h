#pragma once

#include <vector>
#include <utility>
#include <memory>

#include "tntn/tntn_assert.h"
#include "tntn/util.h"

namespace tntn {

//forward declare ObjPool for use in pool_ptr
template<typename T>
class ObjPool;

template<typename T>
class pool_ptr
{
  public:
    pool_ptr() = default;
    pool_ptr(ObjPool<T>* pool, size_t index) : m_pool(pool), m_index(index) {}

    pool_ptr(const pool_ptr& other) = default;
    pool_ptr& operator=(const pool_ptr& other) = default;

    T& operator*() const;
    T* operator->() const;
    T* get() const;

    ObjPool<T>* pool() const noexcept { return m_pool; }
    size_t index() const noexcept { return m_index; }

    void clear() noexcept
    {
        m_pool = nullptr;
        m_index = invalid_index;
    }

    void recycle()
    {
        if(m_pool)
        {
            m_pool->recycle(*this);
            clear();
        }
    }

    bool is_valid() const noexcept { return m_pool && m_pool->contains(*this); }

    explicit operator bool() const noexcept { return is_valid(); }

    bool operator==(const pool_ptr& other) const noexcept
    {
        return m_pool == other.m_pool && m_index == other.m_index;
    }
    bool operator!=(const pool_ptr& other) const noexcept { return !(*this == other); }
    //for use in std::map/std::set
    bool operator<(const pool_ptr& other) const noexcept
    {
        if(m_pool < other.m_pool) return true;
        if(m_pool > other.m_pool) return false;
        if(m_index < other.m_index) return true;
        return false;
    }

  private:
    static constexpr size_t invalid_index = ~(static_cast<size_t>(0));
    friend class ObjPool<T>;
    ObjPool<T>* m_pool = nullptr;
    size_t m_index = invalid_index;
};

template<typename T>
class ObjPool
{
  private:
    //disallow copy, assign and move
    ObjPool(const ObjPool& other) = delete;
    ObjPool(ObjPool&& other) = delete;
    ObjPool& operator=(const ObjPool& other) = delete;
    ObjPool& operator=(ObjPool&& other) = delete;

    //private ctor, use create() static method
    ObjPool() = delete;
    struct private_tag
    {
    };

  public:
    static std::shared_ptr<ObjPool> create() { return std::make_shared<ObjPool>(private_tag()); }

    void reserve(size_t capacity) { m_pool.reserve(capacity); }

    template<typename... Args>
    pool_ptr<T> spawn(Args&&... args)
    {
        m_pool.emplace_back(std::forward<Args>(args)...);
        return pool_ptr<T>(this, m_pool.size() - 1);
    }

    void recycle(const pool_ptr<T>& p)
    {
        //noop until now - should be implemented with some kind of garbage collection and erase/remove-idiom
    }

    bool contains(const pool_ptr<T>& p) const noexcept
    {
        return p.m_pool == this && p.m_index < m_pool.size();
    }

    //do not use this ctor, it's a workaround to make std::make_shared work
    explicit ObjPool(const private_tag) {}

  private:
    friend class pool_ptr<T>;
    friend class pool_ptr<const T>;

    T* get_addr(size_t index) const
    {
        TNTN_ASSERT(index < m_pool.size());
        return const_cast<T*>(&m_pool[index]);
    }

    std::vector<T> m_pool;
};

template<typename T>
inline T* pool_ptr<T>::get() const
{
    return m_pool->get_addr(m_index);
}

template<typename T>
inline T& pool_ptr<T>::operator*() const
{
    return *(get());
}

template<typename T>
inline T* pool_ptr<T>::operator->() const
{
    return get();
}

} //namespace tntn

namespace std {
template<typename T>
struct hash<::tntn::pool_ptr<T>>
{
    std::size_t operator()(const ::tntn::pool_ptr<T>& p) const noexcept
    {
        size_t seed = 0;
        ::tntn::hash_combine(seed, p.pool());
        ::tntn::hash_combine(seed, p.index());
        return seed;
    }
};
} //namespace std
