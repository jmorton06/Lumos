#pragma once
#include "Core/Mutex.h"
#include "TDArray.h"

namespace Lumos
{
    template <typename T>
    class TSQueue
    {
    public:
        TSQueue(size_t cap = 1024)
            : head(0)
            , tail(0)
            , capacity(cap)
        {
            buffer.Resize(capacity);
            locker = new Mutex();
            MutexInit(locker);
        }

        ~TSQueue()
        {
            MutexDestroy(locker);
            delete locker;
        }

        bool Empty()
        {
            ScopedMutex lock(locker);
            return head == tail;
        }

        T& Front()
        {
            return buffer[head];
        }

        bool PushBack(const T& item)
        {
            ScopedMutex lock(locker);
            size_t next = (tail + 1) % capacity;
            if(next == head)
                return false;
            buffer[tail] = item;
            tail         = next;
            return true;
        }

        bool PopFront(T& out)
        {
            ScopedMutex lock(locker);
            if(head == tail)
                return false;
            out  = Move(buffer[head]);
            head = (head + 1) % capacity;
            return true;
        }

        bool PopFront()
        {
            ScopedMutex lock(locker);
            if(head == tail)
                return false;

            head = (head + 1) % capacity;
            return true;
        }

    private:
        TDArray<T> buffer;
        size_t head;
        size_t tail;
        size_t capacity;
        Mutex* locker;
    };
}
