#pragma once

#include <memory>
#include <mutex>
#include <initializer_list>
#include <condition_variable>

template <typename T>
class FineQueue
{
public:

    FineQueue();
    FineQueue(std::initializer_list<T> list);

    FineQueue(const FineQueue& other) = delete;
    FineQueue(FineQueue&& other) = delete;

    FineQueue& operator=(const FineQueue& other) = delete;
    FineQueue& operator=(FineQueue&& other) = delete;

    ~FineQueue() noexcept = default;

    void push(T value);

    template <typename... Args>
    void emplace(Args&&... args);

    std::shared_ptr<T> try_pop();
    bool try_pop(T& value);

    std::shared_ptr<T> wait_and_pop();
    void wait_and_pop(T& value);

    bool empty() const;

    template <typename U>
    friend bool operator==(const FineQueue<U>& one, const FineQueue<U>& other);

private:

    struct Node
    {
        std::shared_ptr<T> data_;
        std::unique_ptr<Node> next_;
    };

    const Node* get_tail() const;

    void push_tail(std::shared_ptr<T> data);

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
        tail_->data_ = std::move(std::make_shared<T>(value));
        tail_->next_ = std::move(std::make_unique<Node>());
        tail_ = tail_->next_.get();
    }
}

template <typename T>
void FineQueue<T>::push(T value)
{
    std::shared_ptr<T> data{ std::make_shared<T>(std::move(value)) };
    push_tail(std::move(data));
}

template <typename T>
template <typename... Args>
void FineQueue<T>::emplace(Args&&... args)
{
    std::shared_ptr<T> data{ std::make_shared<T>(std::forward<Args>(args)...) };
    push_tail(std::move(data));
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
void FineQueue<T>::push_tail(std::shared_ptr<T> data)
{
    std::unique_ptr<Node> next{ std::make_unique<Node>() };
    Node* const tail{ next.get() };

    {
        std::lock_guard<std::mutex> lock{ tailMutex_ };
        tail_->data_ = std::move(data);
        tail_->next_ = std::move(next);
        tail_ = tail;
    }

    isPoppable_.notify_one();
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