#include "gtest/gtest.h"

#include <thread>
#include <chrono>
#include <tuple>

#include <BluntQueue.hpp>
#include <FineQueue.hpp>
#include <ThreadStorage.h>

TEST(BluntQueueTests, DefaultConstruction)
{
    BluntQueue<int> queue{};
}

TEST(BluntQueueTests, InitializerListConstruction)
{
    auto list = { 8, 13, 62 };
    BluntQueue<int> queue = list;

    BluntQueue<int> expected = list;
    ASSERT_EQ(queue, expected) << "Expecting initializer list values properly transfered\n";
}

TEST(BluntQueueTests, CopyConstruction)
{
    BluntQueue<int> donor = { 7, 2, 9, 2 };

    BluntQueue<int> copy{ donor };

    ASSERT_EQ(copy, donor) << "Expecting a copy to fully resemble the donor\n";
} 

TEST(BluntQueueTests, MoveConstruction)
{
     BluntQueue<int> moved{ std::move(BluntQueue<int>{ 1, 3 }) };

     BluntQueue<int> expected = { 1, 3 };
     ASSERT_EQ(moved, expected) << "Expecting a moved to contain original values\n";
}

TEST(BluntQueueTests, CopyAssigned)
{
    BluntQueue<int> source = { 5, 3, 4, 6 };
    BluntQueue<int> target = { 1, 2, 3, 4, 5 };

    target = source;

    ASSERT_EQ(target, source) << "Expecting an assigned to resemble the donor\n";
}

TEST(BluntQueueTests, MoveAssigned)
{
    BluntQueue<int> source = { 8, 5, 7, 1 };
    BluntQueue<int> copy{ source };
    BluntQueue<int> target = { 9, 8 };

    target = std::move(source);

    ASSERT_EQ(target, copy) << "Expecting a move assigned to contain original values\n";
}

TEST(BluntQueueTests, Push)
{
    BluntQueue<int> queue{};

    const int v1{ 0 };
    queue.push(v1);

    BluntQueue<int> expected = { v1 };
    ASSERT_EQ(expected, queue) << "Expecting all values pushed to the BluntQueue\n";
}

TEST(BluntQueueTests, Emplace)
{
    using Tuple = std::tuple<char, int, double>;

    BluntQueue<Tuple> queue{};

    const Tuple v1{ 'a', 1, 1.1 };
    queue.emplace(v1);

    BluntQueue<Tuple> expected = { v1 };
    ASSERT_EQ(expected, queue) << "Expecting all values pushed to the BluntQueue\n";
}

TEST(BluntQueueTests, EmptyRefTryPop)
{
    BluntQueue<int> queue{};

    int front{};
    const bool responce = queue.try_pop(front);

    ASSERT_EQ(false, responce) << "Expecting a failed attempt to pop from an empty BluntQueue\n";
}

TEST(BluntQueueTests, FilledRefTryPop)
{
    const int v1{ 56 };
    BluntQueue<int> queue = { v1, 12, 90 };

    int front{};
    const bool responce = queue.try_pop(front);

    ASSERT_EQ(true, responce) << "Expecting a successful attempt to pop from a BluntQueue\n";
    ASSERT_EQ(v1, front) << "Expecting exact match with the front value\n";
}

TEST(BluntQueueTests, EmptyValueTryPop)
{
    BluntQueue<int> queue{};

    const auto front = queue.try_pop();

    ASSERT_FALSE(front) << "Expecting a failed attempt to pop from an empty BluntQueue\n";
}

TEST(BluntQueueTests, WaitPushAndRefPop)
{
    BluntQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            queue.push(10);
            queue.push(20);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            int front{};
            queue.wait_and_pop(front);
        } };
    }

    ASSERT_EQ(1, queue.size()) << "Expecting a BluntQueue to have one less value\n";
}

TEST(BluntQueueTests, WaitPushAndValuePop)
{
    BluntQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            queue.push(10);
            queue.push(20);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.wait_and_pop();
        } };
    }

    ASSERT_EQ(1, queue.size()) << "Expecting a BluntQueue to have one less value\n";
}

TEST(BluntQueueTests, WaitCopyAssignAndValuePop)
{
    BluntQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            BluntQueue<int> donor = { 13, 8 };
            queue = donor;
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.wait_and_pop();
        } };
    }

    ASSERT_EQ(1, queue.size()) << "Expecting a BluntQueue to have one less value\n";
}

TEST(BluntQueueTests, WaitMoveAssignAndValuePop)
{
    BluntQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            queue = std::move(BluntQueue<int>{ 7, 2 });
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.wait_and_pop();
        } };
    }

    ASSERT_EQ(1, queue.size()) << "Expecting a BluntQueue to have one less value\n";
}

TEST(BluntQueueTests, Size)
{
    const auto list = { 0, 9, 1, 8, 2, 7, 3, 6, 4, 5 };
    BluntQueue<int> queue = list;

    ASSERT_EQ(list.size(), queue.size()) << "Expecting a BluntQueue to take all values into account\n";
}

TEST(BluntQueueTests, Swap)
{
    BluntQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            BluntQueue<int> other = { 1, 2, 3 };
            queue.swap(other);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.wait_and_pop();
        } };
    }

    ASSERT_EQ(2, queue.size())
        << "Expecting the BluntQueue to have an element less after a swap and a pop";
}

TEST(FineQueueTests, DefaultConstruction)
{
    FineQueue<int> queue{};
}

TEST(FineQueueTests, Push)
{
    FineQueue<int> queue{};

    const int v1{ 8 }, v2{ 13 }, v3{ 62 };
    queue.push(v1);
    queue.push(v2);
    queue.push(v3);

    FineQueue<int> reference = { v1, v2, v3 };

    ASSERT_EQ(queue, reference) << "Expecting a FineQueue to hold all pushed elements\n";
}

TEST(FineQueueTests, ParallelPush)
{
    constexpr int v1{ 8 }, v2{ 13 };
    FineQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&queue]()
        {
            queue.push(v1);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.push(v2);
        } };
    }

    FineQueue<int> reference1 = { v1, v2 };
    FineQueue<int> reference2 = { v2, v1 };
    ASSERT_TRUE(queue == reference1 || queue == reference2)
        << "Expecting a FineQueue to contain all pushed elements\n";
}

TEST(FineQueueTests, EmptyValueTryPop)
{
    FineQueue<int> queue{};

    const auto front = queue.try_pop();

    ASSERT_FALSE(front) << "Expecting a failed attempt to pop from an empty FineQueue\n";
}

TEST(FineQueueTests, FilledValueTryPop)
{
    constexpr int v1{ 8 }, v2{ 13 }, v3{ 62 };
    FineQueue<int> queue = { v1, v2, v3 };

    const auto p1 = queue.try_pop();
    const auto p2 = queue.try_pop();
    const auto p3 = queue.try_pop();
    const auto p4 = queue.try_pop();

    ASSERT_EQ(*p1, v1) << "Expecting the first element to show up first\n";
    ASSERT_EQ(*p2, v2) << "Expecting the second element to show up second\n";
    ASSERT_EQ(*p3, v3) << "Expecting the third element to show up third\n";
    ASSERT_FALSE(p4) << "Expecting an empty queue afterwards\n";
}

TEST(FineQueueTests, ParallelValueTryPop)
{
    constexpr int v1{ 7 }, v2{ 8 }, v3{ 9 }, v4{ 10 };
    FineQueue<int> queue = { 1, 2, 3, 4, 5, 6, v1, v2, v3, v4 };
    {
        ThreadStorage threads{ 3u };
        threads[0] = std::thread{ [&queue]()
        {
            queue.try_pop();
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.try_pop();
            queue.try_pop();
            queue.try_pop();
        } };
        threads[2] = std::thread{ [&queue]()
        {
            queue.try_pop();
            queue.try_pop();
        } };
    }

    FineQueue<int> reference = { v1, v2, v3, v4 };
    ASSERT_EQ(queue, reference) << "Expecting 4 elements to survive after parallel pops\n";
}