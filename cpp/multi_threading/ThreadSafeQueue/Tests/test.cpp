#include "pch.h"

#include <thread>
#include <chrono>

#include <ThreadSafeQueue.hpp>
#include <ThreadStorage.h>

TEST(Tests, DefaultConstruction)
{
    ThreadSafeQueue<int> queue{};
}

TEST(Tests, InitializerListConstruction)
{
    auto list = { 8, 13, 62 };
    ThreadSafeQueue<int> queue = list;

    ThreadSafeQueue<int> expected = list;
    ASSERT_EQ(queue, expected) << "Expecting initializer list values properly transfered\n";
}

TEST(Tests, CopyConstruction)
{
    ThreadSafeQueue<int> donor = { 7, 2, 9, 2 };

    ThreadSafeQueue<int> copy{ donor };

    ASSERT_EQ(copy, donor) << "Expecting a copy to fully resemble the donor\n";
} 

TEST(Tests, MoveConstruction)
{
     ThreadSafeQueue<int> moved{ std::move(ThreadSafeQueue<int>{ 1, 3 }) };

     ThreadSafeQueue<int> expected = { 1, 3 };
     ASSERT_EQ(moved, expected) << "Expecting a moved to contain original values\n";
}

TEST(Tests, CopyAssigned)
{
    ThreadSafeQueue<int> source = { 5, 3, 4, 6 };
    ThreadSafeQueue<int> target = { 1, 2, 3, 4, 5 };

    target = source;

    ASSERT_EQ(target, source) << "Expecting an assigned to resemble the donor\n";
}

TEST(Tests, MoveAssigned)
{
    ThreadSafeQueue<int> source = { 8, 5, 7, 1 };
    ThreadSafeQueue<int> copy{ source };
    ThreadSafeQueue<int> target = { 9, 8 };

    target = std::move(source);

    ASSERT_EQ(target, copy) << "Expecting a move assigned to contain original values\n";
}

TEST(Tests, Push)
{
    ThreadSafeQueue<int> queue{};

    const int v1{ 0 };
    queue.push(v1);

    ThreadSafeQueue<int> expected = { v1 };
    ASSERT_EQ(expected, queue) << "Expecting all values pushed to the queue\n";
}

TEST(Tests, EmptyRefTryPop)
{
    ThreadSafeQueue<int> queue{};

    int front{};
    const bool responce = queue.try_pop(front);

    ASSERT_EQ(false, responce) << "Expecting a failed attempt to pop from an empty queue\n";
}

TEST(Tests, FilledRefTryPop)
{
    const int v1{ 56 };
    ThreadSafeQueue<int> queue = { v1, 12, 90 };

    int front{};
    const bool responce = queue.try_pop(front);

    ASSERT_EQ(true, responce) << "Expecting a successful attempt to pop from a queue\n";
    ASSERT_EQ(v1, front) << "Expecting exact match with the front value\n";
}

TEST(Tests, EmptyValueTryPop)
{
    ThreadSafeQueue<int> queue{};

    const auto front = queue.try_pop();

    ASSERT_FALSE(front) << "Expecting a failed attempt to pop from an empty queue\n";
}

TEST(Tests, WaitPushAndRefPop)
{
    ThreadSafeQueue<int> queue{};

    ThreadStorage threads{ 2u };
    threads[0] = std::thread{ [&queue]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        queue.push(10);
    } };
    threads[1] = std::thread{ [&queue]()
    {
        int front{};
        queue.wait_and_pop(front);
    } };

    ASSERT_TRUE(queue.empty()) << "Expecting a queue to be empty\n";
}

TEST(Tests, WaitPushAndValuePop)
{
    ThreadSafeQueue<int> queue{};

    ThreadStorage threads{ 2u };
    threads[0] = std::thread{ [&queue]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        queue.push(10);
    } };
    threads[1] = std::thread{ [&queue]()
    {
        queue.wait_and_pop();
    } };

    ASSERT_TRUE(queue.empty()) << "Expecting a queue to be empty\n";
}

TEST(Tests, WaitCopyAssignAndValuePop)
{
    ThreadSafeQueue<int> queue{};

    ThreadStorage threads{ 2u };
    threads[0] = std::thread{ [&queue]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        ThreadSafeQueue<int> donor = { 13 };
        queue = donor;
    } };
    threads[1] = std::thread{ [&queue]()
    {
        queue.wait_and_pop();
    } };

    ASSERT_TRUE(queue.empty()) << "Expecting a queue to be empty\n";
}

TEST(Tests, WaitMoveAssignAndValuePop)
{
    ThreadSafeQueue<int> queue{};

    ThreadStorage threads{ 2u };
    threads[0] = std::thread{ [&queue]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        queue = std::move(ThreadSafeQueue<int>{ 7 });
    } };
    threads[1] = std::thread{ [&queue]()
    {
        queue.wait_and_pop();
    } };

    ASSERT_TRUE(queue.empty()) << "Expecting a queue to be empty\n";
}