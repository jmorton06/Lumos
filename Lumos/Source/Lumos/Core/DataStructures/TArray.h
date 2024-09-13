#pragma once
#include "Core/OS/Memory.h"

namespace Lumos
{
    template <class T, size_t _Size>
    class TArray
    {
    public:
        // Constructors
        TArray(Arena* arena = nullptr);
        TArray(const TArray<T, _Size>& other);
        TArray(TArray<T, _Size>&& other) noexcept;
        TArray(std::initializer_list<T> values, Arena* arena = nullptr);

        // Destructor
        ~TArray()
        {
            Destroy();
        }

        // Copy assignment
        TArray<T, _Size>& operator=(const TArray<T, _Size>& other);
        // Move assignment
        TArray<T, _Size>& operator=(TArray<T, _Size>&& other) noexcept;

        // Element access
        T& operator[](size_t index);
        const T& operator[](size_t index) const;

        T& Front();
        const T& Front() const;

        T& Back();
        const T& Back() const;

        // Capacity
        bool Empty() const noexcept;
        size_t Size() const noexcept;
        size_t Capacity() const noexcept;
        void Clear() noexcept;
        void Destroy() noexcept;

        class Iterator
        {
        public:
            Iterator(T* ptr)
                : ptr(ptr)
            {
            }
            Iterator operator++()
            {
                ++ptr;
                return *this;
            }
            bool operator!=(const Iterator& other) const { return ptr != other.ptr; }
            T& operator*() const { return *ptr; }
            Iterator operator+(int offset) const { return Iterator(ptr + offset); }

        private:
            T* ptr;
        };

        class ConstIterator
        {
        public:
            ConstIterator(const T* ptr)
                : ptr(ptr)
            {
            }
            ConstIterator operator++()
            {
                ++ptr;
                return *this;
            }
            bool operator!=(const ConstIterator& other) const { return ptr != other.ptr; }
            const T& operator*() const { return *ptr; }
            ConstIterator operator+(int offset) const { return ConstIterator(ptr + offset); }

        private:
            const T* ptr;
        };

        Iterator begin() { return Iterator(m_Data); }
        Iterator end() { return Iterator(m_Data + m_Size); }

        ConstIterator begin() const { return ConstIterator(m_Data); }
        ConstIterator end() const { return ConstIterator(m_Data + m_Size); }

        T* Data() { return m_Data; }

    private:
        T* m_Data;
        size_t m_Size  = 0;
        Arena* m_Arena = nullptr;
    };

    // Constructor implementations
    template <class T, size_t _Size>
    TArray<T, _Size>::TArray(Arena* arena)
        : m_Arena(arena)
        , m_Size(_Size)
    {
        m_Data = nullptr;
        if(m_Arena)
            m_Data = PushArrayNoZero(m_Arena, T, m_Size);
        else
            m_Data = new T[m_Size];
    }

    template <class T, size_t _Size>
    TArray<T, _Size>::TArray(const TArray<T, _Size>& other)
        : m_Size(other.m_Size)
        , m_Arena(other.m_Arena)
    {
        m_Data = nullptr;
        if(m_Arena)
            m_Data = PushArrayNoZero(m_Arena, T, m_Size);
        else
            m_Data = new T[m_Size];

        for(size_t i = 0; i < m_Size; ++i)
        {
            m_Data[i] = other.m_Data[i];
        }
    }

    template <class T, size_t _Size>
    TArray<T, _Size>::TArray(TArray<T, _Size>&& other) noexcept
        : m_Size(other.m_Size)
        , m_Arena(other.m_Arena)
    {
        m_Data = nullptr;
        if(m_Arena)
            m_Data = PushArrayNoZero(m_Arena, T, m_Size);
        else
            m_Data = new T[m_Size];

        for(size_t i = 0; i < m_Size; ++i)
        {
            m_Data[i] = Move(other.m_Data[i]);
        }
        other.m_Size = 0;
    }

    template <class T, size_t _Size>
    TArray<T, _Size>::TArray(std::initializer_list<T> values, Arena* arena)
        : m_Size(_Size)
        , m_Arena(arena)
    {
        m_Data = nullptr;
        if(m_Arena)
            m_Data = PushArrayNoZero(m_Arena, T, m_Size);
        else
            m_Data = new T[m_Size];
        size_t index = 0;
        for(auto& value : values)
            m_Data[index++] = value;
    }

    // Copy assignment implementation
    template <class T, size_t _Size>
    TArray<T, _Size>& TArray<T, _Size>::operator=(const TArray<T, _Size>& other)
    {
        if(this != &other)
        {
            m_Size  = other.m_Size;
            m_Arena = other.m_Arena;
            for(size_t i = 0; i < m_Size; ++i)
            {
                m_Data[i] = other.m_Data[i];
            }
        }
        return *this;
    }

    // Move assignment implementation
    template <class T, size_t _Size>
    TArray<T, _Size>& TArray<T, _Size>::operator=(TArray<T, _Size>&& other) noexcept
    {
        if(this != &other)
        {
            m_Size  = other.m_Size;
            m_Arena = other.m_Arena;

            m_Data = nullptr;
            if(m_Arena)
                m_Data = PushArrayNoZero(m_Arena, T, m_Size);
            else
                m_Data = new T[m_Size];

            for(size_t i = 0; i < m_Size; ++i)
            {
                m_Data[i] = Move(other.m_Data[i]);
            }
            other.m_Size = 0;
        }
        return *this;
    }

    // Element access implementations
    template <class T, size_t _Size>
    T& TArray<T, _Size>::operator[](size_t index)
    {
        ASSERT(index < m_Size, "Index must be less than array's size");
        return m_Data[index];
    }

    template <class T, size_t _Size>
    const T& TArray<T, _Size>::operator[](size_t index) const
    {
        ASSERT(index < m_Size, "Index must be less than array's size");
        return m_Data[index];
    }

    template <class T, size_t _Size>
    T& TArray<T, _Size>::Front()
    {
        return *begin();
    }

    template <class T, size_t _Size>
    const T& TArray<T, _Size>::Front() const
    {
        return *begin();
    }

    template <class T, size_t _Size>
    T& TArray<T, _Size>::Back()
    {
        return m_Data[m_Size - 1];
    }

    template <class T, size_t _Size>
    const T& TArray<T, _Size>::Back() const
    {
        return *(end() - 1);
    }

    // Capacity implementations
    template <class T, size_t _Size>
    bool TArray<T, _Size>::Empty() const noexcept
    {
        return m_Size == 0;
    }

    template <class T, size_t _Size>
    size_t TArray<T, _Size>::Size() const noexcept
    {
        return m_Size;
    }

    template <class T, size_t _Size>
    size_t TArray<T, _Size>::Capacity() const noexcept
    {
        return Size;
    }

    // Modifiers implementations
    template <class T, size_t _Size>
    void TArray<T, _Size>::Clear() noexcept
    {
        m_Size = 0;
    }

    template <class T, size_t _Size>
    void TArray<T, _Size>::Destroy() noexcept
    {
        if(m_Data)
        {
            if(m_Arena)
            {
                for(size_t i = 0; i < m_Size; ++i)
                {
                    m_Data[i].~T();
                }
            }
            else
            {
                delete[] m_Data;
            }
        }

        m_Data = nullptr;
        m_Size = 0;
    }
}
