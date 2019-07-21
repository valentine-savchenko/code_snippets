#pragma once

#include <mutex>
#include <condition_variable>
#include <memory>
#include <initializer_list>
#include <deque>
#include <utility>

template <typename T>
class ThreadSafeQueue;

template <typename T>
bool operator==(const ThreadSafeQueue<T>& one, const ThreadSafeQueue<T>& other);

template <typename T>
class ThreadSafeQueue
{
public:
    using size_type = typename std::deque<T>::size_type;

    ThreadSafeQueue() = default;
    ThreadSafeQueue(std::initializer_list<T> items);

    ThreadSafeQueue(const ThreadSafeQueue& other);
    ThreadSafeQueue(ThreadSafeQueue&& other) noexcept;

    ThreadSafeQueue& operator=(const ThreadSafeQueue& other);
    ThreadSafeQueue& operator=(ThreadSafeQueue&& other) noexcept;

    ~ThreadSafeQueue() noexcept = default;

    void push(T value);

    template <typename... Args>
    void emplace(Args&&... args);

    bool try_pop(T& value);
    std::shared_ptr<T> try_pop();

    bool wait_and_pop(T& value);
    std::shared_ptr<T> wait_and_pop();

    bool empty() const;
    size_type size() const;

    void swap(ThreadSafeQueue<T>& other);

    friend bool operator==<T>(const ThreadSafeQueue<T>& left, const ThreadSafeQueue<T>& right);

private:

    mutable std::mutex mutex_;
    std::condition_variable isPopulated_;
    std::deque<T> storage_;

};

template <typename T>
bool operator==(const ThreadSafeQueue<T>& one, const ThreadSafeQueue<T>& other)
{
    std::lock(one.mutex_, other.mutex_);
    std::lock_guard<std::mutex> oneLock{ one.mutex_, std::adopt_lock };
    std::lock_guard<std::mutex> otherLock{ other.mutex_, std::adopt_lock };
    return one.storage_ == other.storage_;
}

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(std::initializer_list<T> items) :
    mutex_{},
    isPopulated_{},
    storage_{ items.begin(), items.end() }
{
    // Empty
}

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(const ThreadSafeQueue& other) :
    mutex_{},
    isPopulated_{}
{
    std::lock_guard<std::mutex> lock{ other.mutex_ };
    storage_.assign(other.storage_.cbegin(), other.storage_.cend());
}

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(ThreadSafeQueue&& other) noexcept :
    mutex_{},
    isPopulated_{},
    storage_{ std::move(other.storage_) }
{
    // Empty
}

template <typename T>
ThreadSafeQueue<T>& ThreadSafeQueue<T>::operator=(const ThreadSafeQueue& other)
{
    {
        std::lock(mutex_, other.mutex_);
        std::lock_guard<std::mutex> lock{ mutex_, std::adopt_lock };
        std::lock_guard<std::mutex> otherLock{ other.mutex_, std::adopt_lock };
        storage_.assign(other.storage_.begin(), other.storage_.end());
    }
    isPopulated_.notify_one();
    return *this;
}

template <typename T>
ThreadSafeQueue<T>& ThreadSafeQueue<T>::operator=(ThreadSafeQueue&& other) noexcept
{
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        storage_ = std::move(other.storage_);
    }
    isPopulated_.notify_one();
    return *this;
}

template <typename T>
void ThreadSafeQueue<T>::push(T value)
{
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        storage_.push_back(std::move(value));
    }
    isPopulated_.notify_one();
}

template <typename T>
template <typename... Args>
void ThreadSafeQueue<T>::emplace(Args&& ... args)
{
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        storage_.emplace_back(std::forward<Args>(args)...);
    }
    isPopulated_.notify_one();
}

template <typename T>
bool ThreadSafeQueue<T>::try_pop(T& value)
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    if (storage_.empty())
    {
        return false;
    }

    value = storage_.front();
    storage_.pop_front();
    return true;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::try_pop()
{
    std::shared_ptr<T> value{};

    std::lock_guard<std::mutex> lock{ mutex_ };
    if (storage_.empty())
    {
        return value;
    }

    value.reset(new T{ storage_.front() });
    storage_.pop_front();
    return value;
}

template <typename T>
bool ThreadSafeQueue<T>::wait_and_pop(T& value)
{
    std::unique_lock<std::mutex> lock{ mutex_ };
    isPopulated_.wait(lock, [this]() { return !storage_.empty(); });

    value = storage_.front();
    storage_.pop_front();
    return true;
}

template <typename T>
std::shared_ptr<T> ThreadSafeQueue<T>::wait_and_pop()
{
    std::shared_ptr<T> value{};

    std::unique_lock<std::mutex> lock{ mutex_ };
    isPopulated_.wait(lock, [this]() { return !storage_.empty(); });

    value.reset(new T{ storage_.front() });
    storage_.pop_front();
    return value;
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    return storage_.empty();
}

template <typename T>
typename ThreadSafeQueue<T>::size_type ThreadSafeQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    return storage_.size();
}

template <typename T>
void ThreadSafeQueue<T>::swap(ThreadSafeQueue& other)
{
    {
        std::lock(mutex_, other.mutex_);
        std::lock_guard<std::mutex> lock{ mutex_, std::adopt_lock };
        std::lock_guard<std::mutex> otherLock{ other.mutex_, std::adopt_lock };
        storage_.swap(other.storage_);
    }
    isPopulated_.notify_one();
    other.isPopulated_.notify_one();
}