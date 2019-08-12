#pragma once

#include <cstddef>
#include <memory>
#include <mutex>
#include <initializer_list>
#include <condition_variable>
#include <utility>

template <typename T>
class FineQueue
{
public:
    using size_type = std::size_t;

    FineQueue();
    FineQueue(std::initializer_list<T> list);

    FineQueue(const FineQueue& other);
    FineQueue(FineQueue&& other) noexcept;

    FineQueue& operator=(const FineQueue& other);
    FineQueue& operator=(FineQueue&& other) noexcept;

    ~FineQueue() noexcept = default;

    void push(T value);

    template <typename... Args>
    void emplace(Args&&... args);

    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);

    std::shared_ptr<T> wait_and_pop();
    void wait_and_pop(T& value);

    size_type size() const;
    bool empty() const;

    void swap(FineQueue& other) noexcept;

    template <typename U>
    friend bool operator==(const FineQueue<U>& one, const FineQueue<U>& other);

private:

    struct Node
    {
        std::shared_ptr<T> data_;
        std::unique_ptr<Node> next_;
    };

    const Node* get_tail() const;
    void populate(std::shared_ptr<T> data, std::unique_ptr<Node> next, Node*& tail);
    void lock_push_tail(std::shared_ptr<T> data);
    void lock_traverse_push(const FineQueue& other, Node*& tail);

    std::shared_ptr<T> subscribe_head_data();
    void steal_head_data(T& value);
    std::unique_lock<std::mutex> wait_for_head();
    void pop_head();

    std::condition_variable isPoppable_;

    mutable std::mutex headMutex_;
    std::unique_ptr<Node> head_;

    mutable std::mutex tailMutex_;
    Node* tail_;

};

template <typename T>
FineQueue<T>::FineQueue() :
    isPoppable_{},
    headMutex_{},
    head_{ std::make_unique<Node>() },
    tailMutex_{},
    tail_{ head_.get() }
{
    // Empty
}

template <typename T>
FineQueue<T>::FineQueue(std::initializer_list<T> list) :
    isPoppable_{},
    headMutex_{},
    head_{ std::make_unique<Node>() },
    tailMutex_{},
    tail_{ head_.get() }
{
    for (const T value : list)
    {
        std::shared_ptr<T> data{ std::make_shared<T>(value) };
        std::unique_ptr<Node> next{ std::make_unique<Node>() };
        populate(std::move(data), std::move(next), tail_);
    }
}

template <typename T>
FineQueue<T>::FineQueue(const FineQueue& other) :
    isPoppable_{},
    headMutex_{},
    head_{ std::make_unique<Node>() },
    tailMutex_{},
    tail_{ head_.get() }
{
    lock_traverse_push(other, tail_);
}

template <typename T>
FineQueue<T>::FineQueue(FineQueue&& other) noexcept :
    isPoppable_{},
    headMutex_{},
    head_{ std::move(other.head_) },
    tailMutex_{},
    tail_{ std::move(other.tail_) }
{
    // It is caller's responsibility to ensure that the source queue
    // is not being modified at any other thread
    // (which is kind of extravagant to do in the first place)
}

template <typename T>
FineQueue<T>& FineQueue<T>::operator=(const FineQueue& other)
{
    if (this == &other)
    {
        return *this;
    }

    // Clone elements of the source queue
    // to provide the strong exception safety for the content
    // Allow the queue to be extended (but not shrunk) while traversing it
    std::unique_ptr<Node> head{ std::make_unique<Node>() };
    Node* tail{ head.get() };
    lock_traverse_push(other, tail);

    // Prevent any modifications to the content, before discarding it
    {
        std::scoped_lock<std::mutex, std::mutex> lock{ headMutex_, tailMutex_ };
        head_ = std::move(head);
        tail_ = std::move(tail);
    }
    isPoppable_.notify_one();

    return *this;
}

template <typename T>
FineQueue<T>& FineQueue<T>::operator=(FineQueue&& other) noexcept
{
    // It is caller's responsibility to ensure that the source queue
    // is not being modified at any other thread
    // (which is kind of extravagant to do in the first place)

    // Prevent any modifications to the content, before discarding it
    {
        std::scoped_lock<std::mutex, std::mutex> lock{ headMutex_, tailMutex_ };
        head_ = std::move(other.head_);
        tail_ = std::move(other.tail_);
    }
    isPoppable_.notify_one();

    return *this;
}

template <typename T>
void FineQueue<T>::push(T value)
{
    std::shared_ptr<T> data{ std::make_shared<T>(std::move(value)) };
    lock_push_tail(std::move(data));
}

template <typename T>
template <typename... Args>
void FineQueue<T>::emplace(Args&&... args)
{
    std::shared_ptr<T> data{ std::make_shared<T>(std::forward<Args>(args)...) };
    lock_push_tail(std::move(data));
}

template <typename T>
std::shared_ptr<T> FineQueue<T>::try_pop()
{
    std::lock_guard<std::mutex> lock{ headMutex_ };
    if (head_.get() == get_tail())
    {
        return std::shared_ptr<T>{};
    }

    std::shared_ptr<T> data{ subscribe_head_data() };
    pop_head();
    return data;
}

template <typename T>
bool FineQueue<T>::try_pop(T& value)
{
    std::lock_guard<std::mutex> lock{ headMutex_ };
    if (head_.get() == get_tail())
    {
        return false;
    }

    steal_head_data(value);
    pop_head();
    return true;
}

template <typename T>
std::shared_ptr<T> FineQueue<T>::wait_and_pop()
{
    std::unique_lock<std::mutex> lock{ wait_for_head() };
    std::shared_ptr<T> data{ subscribe_head_data() };
    pop_head();
    return data;
}

template <typename T>
void FineQueue<T>::wait_and_pop(T& value)
{
    std::unique_lock<std::mutex> lock{ wait_for_head() };
    steal_head_data(value);
    pop_head();
}

template <typename T>
typename FineQueue<T>::size_type FineQueue<T>::size() const
{
    size_type length{ 0 };

    // Allow the queue to expand (but not shrink) while computing its length
    std::lock_guard<std::mutex> lock{ headMutex_ };
    for (const Node* i{ head_.get() }; i != get_tail(); i = i->next_.get())
    {
        ++length;
    }

    return length;
}

template <typename T>
bool FineQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock{ headMutex_ };
    return head_.get() == get_tail();
}

template <typename T>
const typename FineQueue<T>::Node* FineQueue<T>::get_tail() const
{
    std::lock_guard<std::mutex> lock{ tailMutex_ };
    return tail_;
}

template <typename T>
void FineQueue<T>::populate(std::shared_ptr<T> data, std::unique_ptr<Node> next, Node*& tail)
{
    tail->data_ = std::move(data);
    Node* const t{ next.get() };
    tail->next_ = std::move(next);
    tail = t;
}

template <typename T>
void FineQueue<T>::lock_push_tail(std::shared_ptr<T> data)
{
    std::unique_ptr<Node> next{ std::make_unique<Node>() };
    {
        std::lock_guard<std::mutex> lock{ tailMutex_ };
        populate(std::move(data), std::move(next), tail_);
    }

    isPoppable_.notify_one();
}

template <typename T>
void FineQueue<T>::lock_traverse_push(const FineQueue& other, Node*& tail)
{
    // Allow the source queue to be extended (but not shrunk) at other threads, if any 
    std::lock_guard<std::mutex> lock{ other.headMutex_ };
    for (const Node* i{ other.head_.get() }; i != other.get_tail(); i = i->next_.get())
    {
        std::shared_ptr<T> data{ std::make_shared<T>(*i->data_) };
        std::unique_ptr<Node> next{ std::make_unique<Node>() };
        populate(std::move(data), std::move(next), tail);
    }
}

template <typename T>
std::shared_ptr<T> FineQueue<T>::subscribe_head_data()
{
    return head_->data_;
}

template <typename T>
void FineQueue<T>::steal_head_data(T& value)
{
    value = std::move(*(head_->data_));
}

template <typename T>
std::unique_lock<std::mutex> FineQueue<T>::wait_for_head()
{
    std::unique_lock<std::mutex> lock{ headMutex_ };
    isPoppable_.wait(lock, [this]() { return head_.get() != get_tail(); });
    return lock;
}

template <typename T>
void FineQueue<T>::pop_head()
{
    std::unique_ptr<Node> oldHead_{ std::move(head_) };
    head_ = std::move(oldHead_->next_);
}

template <typename T>
void FineQueue<T>::swap(FineQueue& other) noexcept
{
    std::scoped_lock<std::mutex, std::mutex, std::mutex, std::mutex> lock{
        headMutex_, tailMutex_, other.headMutex_, other.tailMutex_ };
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
}

template <typename T>
bool operator==(const FineQueue<T>& one, const FineQueue<T>& other)
{
    using Node = FineQueue<T>::Node;

    std::scoped_lock<std::mutex, std::mutex, std::mutex, std::mutex> lock{
        one.headMutex_, one.tailMutex_, other.headMutex_, other.tailMutex_ };

    const Node* iterOne{ one.head_.get() };
    const Node* iterOther{ other.head_.get() };
    while ((iterOne != one.tail_) && (iterOther != other.tail_) &&
        (*(iterOne->data_) == *(iterOther->data_)))
    {
        iterOne = iterOne->next_.get();
        iterOther = iterOther->next_.get();
    }

    return iterOne == one.tail_ && iterOther == other.tail_;
}