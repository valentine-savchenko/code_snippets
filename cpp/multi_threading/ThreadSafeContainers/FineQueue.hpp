#pragma once

#include <memory>
#include <mutex>
#include <initializer_list>

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

    ~FineQueue() = default;

    void push(T value);
    std::shared_ptr<T> try_pop();

    template <typename U>
    friend bool operator==(const FineQueue<U>& one, const FineQueue<U>& other);

private:

    struct Node
    {
        std::shared_ptr<T> data_;
        std::unique_ptr<Node> next_;
    };

    const Node* get_tail() const;

    mutable std::mutex headMutex_;
    std::unique_ptr<Node> head_;

    mutable std::mutex tailMutex_;
    Node* tail_;

};

template <typename T>
FineQueue<T>::FineQueue() :
    head_{ std::make_unique<Node>() },
    tail_{ head_.get() }
{
    // Empty
}

template <typename T>
FineQueue<T>::FineQueue(std::initializer_list<T> list) :
    head_{ std::make_unique<Node>() },
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
    std::unique_ptr<Node> next{ std::make_unique<Node>() };
    Node* const tail{ next.get() };

    std::lock_guard<std::mutex> lock{ tailMutex_ };
    tail_->data_ = std::move(data);
    tail_->next_ = std::move(next);
    tail_ = tail;
}

template <typename T>
std::shared_ptr<T> FineQueue<T>::try_pop()
{
    std::lock_guard<std::mutex> lock{ headMutex_ };
    if (head_.get() == get_tail())
    {
        return std::shared_ptr<T>{};
    }

    std::shared_ptr<T> data{ head_->data_ };
    std::unique_ptr<Node> oldHead_{ std::move(head_) };
    head_ = std::move(oldHead_->next_);

    return data;
}

template <typename T>
const typename FineQueue<T>::Node* FineQueue<T>::get_tail() const
{
    std::lock_guard<std::mutex> lock{ tailMutex_ };
    return tail_;
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