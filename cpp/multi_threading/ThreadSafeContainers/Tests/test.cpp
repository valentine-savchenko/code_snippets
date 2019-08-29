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

TEST(BluntQueueTests, ParallelMoveConstruction)
{
    BluntQueue<int> donor = { 1, 2, 3, 4, 5 };

    std::thread thread{ [](BluntQueue<int>&& donor)
    {
        BluntQueue<int> owner{ std::move(donor) };
    }, std::move(donor) };

    BluntQueue<int> competitor{ std::move(donor) };

    thread.join();

    ASSERT_TRUE(donor.empty()) << "Expecting a moved from queue to be empty";
    ASSERT_TRUE(competitor.empty()) << "Expecting a competitor to be empty";
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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
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

    const FineQueue<int> reference = { v1, v2, v3 };
    ASSERT_EQ(queue, reference) << "Expecting a queue to hold all pushed elements\n";
}

TEST(FineQueueTests, ParallelPush)
{
    constexpr int v1{ 8 }, v2{ 13 };
    FineQueue<int> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [v1, &queue]()
        {
            queue.push(v1);
        } };
        threads[1] = std::thread{ [v2, &queue]()
        {
            queue.push(v2);
        } };
    }

    const FineQueue<int> reference1 = { v1, v2 };
    const FineQueue<int> reference2 = { v2, v1 };
    ASSERT_TRUE(queue == reference1 || queue == reference2)
        << "Expecting a queue to contain all pushed elements\n";
}

TEST(FineQueueTests, Emplace)
{
    using Tuple = std::tuple<char, int, double>;

    FineQueue<Tuple> queue{};

    const char v1{ 8 };
    const int v2{ 13 };
    const double v3{ 62 };
    queue.emplace(v1, v2, v3);

    const FineQueue<Tuple> reference = { std::make_tuple(v1, v2, v3) };
    ASSERT_EQ(queue, reference) << "Expecting a queue to hold the emplace-ed element\n";
}

TEST(FineQueueTests, ParallelEmplace)
{
    using Tuple = std::tuple<char, int, double>;

    constexpr char v1{ 8 };
    constexpr int v2{ 13 };
    constexpr double v3{ 62 };
    FineQueue<Tuple> queue{};
    {
        ThreadStorage threads{ 2u };
        threads[0] = std::thread{ [&]()
        {
            queue.emplace(v1, v2, v3);
        } };
        threads[1] = std::thread{ [&]()
        {
            queue.emplace(v1, v2, v3);
        } };
    }

    const FineQueue<Tuple> reference = { std::make_tuple(v1, v2, v3),
        std::make_tuple(v1, v2, v3) };
    ASSERT_TRUE(queue == reference)
        << "Expecting a queue to contain all pushed elements\n";
}

TEST(FineQueueTests, EmptyValueTryPop)
{
    FineQueue<int> queue{};

    const auto front = queue.try_pop();

    ASSERT_FALSE(front) << "Expecting a failed attempt to pop from an empty queue\n";
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

    const FineQueue<int> reference = { v1, v2, v3, v4 };
    ASSERT_EQ(queue, reference) << "Expecting 4 elements to survive after parallel pops\n";
}

TEST(FineQueueTests, ParallelRefTryPop)
{
    constexpr int v1{ 7 }, v2{ 8 }, v3{ 9 }, v4{ 10 };
    FineQueue<int> queue = { 1, 2, 3, 4, 5, 6, v1, v2, v3, v4 };
    {
        ThreadStorage threads{ 3u };
        threads[0] = std::thread{ [&queue]()
        {
            int dummy{};
            queue.try_pop(dummy);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            int dummy{};
            queue.try_pop(dummy);
            queue.try_pop(dummy);
            queue.try_pop(dummy);
        } };
        threads[2] = std::thread{ [&queue]()
        {
            int dummy{};
            queue.try_pop(dummy);
            queue.try_pop(dummy);
        } };
    }

    const FineQueue<int> reference = { v1, v2, v3, v4 };
    ASSERT_EQ(queue, reference) << "Expecting 4 elements to survive after parallel pops\n";
}

TEST(FineQueueTests, PushAndWaitValuePop)
{
    FineQueue<int> queue{};
    {
        ThreadStorage threads{ 3u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            queue.push(8);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            queue.push(13);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            queue.wait_and_pop();
        } };
        threads[2] = std::thread{ [&queue]()
        {
            queue.wait_and_pop();
        } };
    }

    ASSERT_TRUE(queue.empty()) << "Expecting a queue to be totally empty\n";
}

TEST(FineQueueTests, PushAndWaitRefPop)
{
    FineQueue<int> queue{};
    {
        ThreadStorage threads{ 3u };
        threads[0] = std::thread{ [&queue]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            queue.push(8);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            queue.push(13);
        } };
        threads[1] = std::thread{ [&queue]()
        {
            int dummy{};
            queue.wait_and_pop(dummy);
        } };
        threads[2] = std::thread{ [&queue]()
        {
            int dummy{};
            queue.wait_and_pop(dummy);
        } };
    }

    ASSERT_TRUE(queue.empty()) << "Expecting a queue to be totally empty\n";
}

TEST(FineQueueTests, ParallelCopyConstruction)
{
    std::initializer_list<int> values = { 8, 13, 62 };
    FineQueue<int> donor = values;

    std::thread pusher{ [&donor]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        donor.push(4);
    } };
    std::thread popper{ [&donor]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        donor.try_pop();
    } };

    FineQueue<int> copy{ donor };

    popper.join();
    pusher.join();

    const FineQueue<int> reference = values;
    ASSERT_EQ(reference, copy)
        << "Expecting a copy to fully resemble initial state of the donor\n";
}

TEST(FineQueueTests, MoveConstruction)
{
    std::initializer_list<int> values = { 8, 13, 62 };

    const FineQueue<int> queue{ std::move(FineQueue<int>(values)) };

    const FineQueue<int> reference = values;
    ASSERT_EQ(queue, reference) << "Expecting a copy to fully resemble the blueprint\n";
}

TEST(FineQueueTests, ParallelCopyAssigned)
{
    constexpr int v1{ 62 }, v2{ 13 }, v3{ 8 };
    FineQueue<int> queue{ v3, v2, v1 };
    FineQueue<int> donor{ v1, v2, v3 };

    constexpr int p1{ 72 }, p2{ 92 };
    std::thread pusher{ [p1, p2, &donor]()
    {
        donor.push(p1);
        donor.push(p2);
    } };
 
    queue = donor;

    pusher.join();

    const FineQueue<int> reference{ v1, v2, v3 };
    const FineQueue<int> reference1{ v1, v2, v3, p1 };
    const FineQueue<int> reference2{ v1, v2, v3, p1, p2 };

    ASSERT_TRUE(reference == queue || reference1 == queue || reference2 == queue)
        << "Expecting a queue to receive all elements of the donor\n";
}

TEST(FineQueueTests, MoveAssigned)
{
    std::initializer_list<int> ref = { 8, 13, 62 };
    FineQueue<int> queue{ 1, 2, 3 };

    queue = std::move(FineQueue<int>{ ref });

    const FineQueue<int> reference{ ref };
    ASSERT_EQ(queue, reference)
        << "Expecting a queue to absorb all elements of the donor\n";
}

TEST(FineQueueTests, Swap)
{
    std::initializer_list<int> init1{ 8, 13, 62 };
    FineQueue<int> queue1{ init1 };
    std::initializer_list<int> init2{ 62, 13, 8 };
    FineQueue<int> queue2{ init2 };

    queue1.swap(queue2);

    const FineQueue<int> reference1{ init2 };
    const FineQueue<int> reference2{ init1 };
    ASSERT_EQ(queue1, reference1) << "Expecting a queue to grab partner's elements";
    ASSERT_EQ(queue2, reference2) << "Expecting a queue to grab partner's elements";
}