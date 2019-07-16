#pragma once

#include <thread>
#include <vector>

class ThreadStorage : private std::vector<std::thread>
{
private:

    using Base = std::vector<std::thread>;

public:

    ThreadStorage() = delete;
    ThreadStorage(const ThreadStorage& other) = delete;
    ThreadStorage(ThreadStorage&& other) = delete;
    ThreadStorage& operator=(const ThreadStorage& other) = delete;
    ThreadStorage& operator=(ThreadStorage&& other) = delete;

    explicit ThreadStorage(const unsigned number);
    ~ThreadStorage();

    using Base::operator[];

};