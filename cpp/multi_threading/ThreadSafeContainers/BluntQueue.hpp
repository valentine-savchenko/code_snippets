#pragma once

#include <mutex>
#include <condition_variable>
#include <memory>
#include <initializer_list>
#include <deque>
#include <utility>

// Forward declaring equality operator to make it a friend of the queue
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

    // Pushing objects, which are not performance critical, by value
    void push(T value);

    // Constructing performance critical objects emplace
    template <typename... Args>
    void emplace(Args&&... args);

    // Providing complete pop operations, rather than the classical front / pop tandem
    // to avoid race conditions inherited in the interface

    // Supplying a version of pop to retrieve values of types,
    // which are cheap to construct or could be constructed beforehand
    bool try_pop(T& value);

    // When it is not practical, providing a version to receive
    // a managed pointer to a value of interest
    std::shared_ptr<T> try_pop();

    // Providing waiting pops for those consumers, which are willing to wait for a value
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
    std::scoped_lock<std::mutex, std::mutex> lock{ one.mutex_, other.mutex_ };
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
    const auto& storage = other.storage_;
    storage_.assign(storage.cbegin(), storage.cend());
}

template <typename T>
BluntQueue<T>::BluntQueue(BluntQueue&& other) noexcept :
    mutex_{},
    isPopulated_{}
{
    std::lock_guard<std::mutex> lock{ other.mutex_ };
    storage_ = std::move(other.storage_);
}

template <typename T>
BluntQueue<T>& BluntQueue<T>::operator=(const BluntQueue& other)
{
    {
        // Reducing the locking scope to let a thread waiting a notification to acquire the mutex faster
        std::scoped_lock<std::mutex, std::mutex> lock{ mutex_, other.mutex_ };
        storage_.assign(other.storage_.begin(), other.storage_.end());
    }
    isPopulated_.notify_one();
    return *this;
}

template <typename T>
BluntQueue<T>& BluntQueue<T>::operator=(BluntQueue&& other) noexcept
{
    {
        std::scoped_lock<std::mutex, std::mutex> lock{ mutex_, other.mutex_ };
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

    // Both moving from and destruction of the top element are expected to be non-throwing,
    // so the strong exception safety are guaranteed
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
        std::scoped_lock<std::mutex, std::mutex> lock{ mutex_, other.mutex_ };
        storage_.swap(other.storage_);
    }
    isPopulated_.notify_one();
    other.isPopulated_.notify_one();
}