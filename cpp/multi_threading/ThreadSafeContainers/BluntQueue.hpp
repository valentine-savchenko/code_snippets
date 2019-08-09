#pragma once

#include <mutex>
#include <condition_variable>
#include <memory>
#include <initializer_list>
#include <deque>
#include <utility>

template <typename T>
class BluntQueue;

template <typename T>
bool operator==(const BluntQueue<T>& one, const BluntQueue<T>& other);

template <typename T>
class BluntQueue
{
public:
    using size_type = typename std::deque<T>::size_type;

    BluntQueue() = default;
    BluntQueue(std::initializer_list<T> items);

    BluntQueue(const BluntQueue& other);
    BluntQueue(BluntQueue&& other) noexcept;

    BluntQueue& operator=(const BluntQueue& other);
    BluntQueue& operator=(BluntQueue&& other) noexcept;

    ~BluntQueue() noexcept = default;

    void push(T value);

    template <typename... Args>
    void emplace(Args&&... args);

    bool try_pop(T& value);
    std::shared_ptr<T> try_pop();

    bool wait_and_pop(T& value);
    std::shared_ptr<T> wait_and_pop();

    bool empty() const;
    size_type size() const;

    void swap(BluntQueue<T>& other);

    friend bool operator==<T>(const BluntQueue<T>& one, const BluntQueue<T>& other);

private:

    mutable std::mutex mutex_;
    std::condition_variable isPopulated_;
    std::deque<T> storage_;

};

template <typename T>
bool operator==(const BluntQueue<T>& one, const BluntQueue<T>& other)
{
    std::lock(one.mutex_, other.mutex_);
    std::lock_guard<std::mutex> oneLock{ one.mutex_, std::adopt_lock };
    std::lock_guard<std::mutex> otherLock{ other.mutex_, std::adopt_lock };
    return one.storage_ == other.storage_;
}

template <typename T>
BluntQueue<T>::BluntQueue(std::initializer_list<T> items) :
    mutex_{},
    isPopulated_{},
    storage_{ items.begin(), items.end() }
{
    // Empty
}

template <typename T>
BluntQueue<T>::BluntQueue(const BluntQueue& other) :
    mutex_{},
    isPopulated_{}
{
    std::lock_guard<std::mutex> lock{ other.mutex_ };
    storage_.assign(other.storage_.cbegin(), other.storage_.cend());
}

template <typename T>
BluntQueue<T>::BluntQueue(BluntQueue&& other) noexcept :
    mutex_{},
    isPopulated_{},
    storage_{ std::move(other.storage_) }
{
    // Empty
}

template <typename T>
BluntQueue<T>& BluntQueue<T>::operator=(const BluntQueue& other)
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
BluntQueue<T>& BluntQueue<T>::operator=(BluntQueue&& other) noexcept
{
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        storage_ = std::move(other.storage_);
    }
    isPopulated_.notify_one();
    return *this;
}

template <typename T>
void BluntQueue<T>::push(T value)
{
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        storage_.push_back(std::move(value));
    }
    isPopulated_.notify_one();
}

template <typename T>
template <typename... Args>
void BluntQueue<T>::emplace(Args&& ... args)
{
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        storage_.emplace_back(std::forward<Args>(args)...);
    }
    isPopulated_.notify_one();
}

template <typename T>
bool BluntQueue<T>::try_pop(T& value)
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    if (storage_.empty())
    {
        return false;
    }

    value = std::move(storage_.front());
    storage_.pop_front();
    return true;
}

template <typename T>
std::shared_ptr<T> BluntQueue<T>::try_pop()
{
    std::shared_ptr<T> value{};

    std::lock_guard<std::mutex> lock{ mutex_ };
    if (storage_.empty())
    {
        return value;
    }

    value.reset(new T{ std::move(storage_.front()) });
    storage_.pop_front();
    return value;
}

template <typename T>
bool BluntQueue<T>::wait_and_pop(T& value)
{
    std::unique_lock<std::mutex> lock{ mutex_ };
    isPopulated_.wait(lock, [this]() { return !storage_.empty(); });

    value = std::move(storage_.front());
    storage_.pop_front();
    return true;
}

template <typename T>
std::shared_ptr<T> BluntQueue<T>::wait_and_pop()
{
    std::shared_ptr<T> value{};

    std::unique_lock<std::mutex> lock{ mutex_ };
    isPopulated_.wait(lock, [this]() { return !storage_.empty(); });

    value.reset(new T{ std::move(storage_.front()) });
    storage_.pop_front();
    return value;
}

template <typename T>
bool BluntQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    return storage_.empty();
}

template <typename T>
typename BluntQueue<T>::size_type BluntQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock{ mutex_ };
    return storage_.size();
}

template <typename T>
void BluntQueue<T>::swap(BluntQueue& other)
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