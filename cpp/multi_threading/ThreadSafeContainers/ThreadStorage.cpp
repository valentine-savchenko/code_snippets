#include "ThreadStorage.h"

ThreadStorage::ThreadStorage(const unsigned number) :
    ThreadStorage::Base(number)
{
    // Empty
}

ThreadStorage::~ThreadStorage()
{
    for (Base::iterator t{ begin() }; t != end(); ++t)
    {
        if (t->joinable())
        {
            t->join();
        }
    }
}