#pragma once
#include "Core/OS/Memory.h"
#include <initializer_list>

namespace Lumos
{
    template <class T>
    class TDArray
    {
    public:
        // Constructors
        TDArray(Arena* arena = nullptr);
        TDArray(const TDArray<T>& other);
        TDArray(TDArray<T>&& other) noexcept;
        TDArray(size_t size, const T& initial = T {}, Arena* arena = nullptr);
        TDArray(std::initializer_list<T> values, Arena* arena = nullptr);

        // Destructor
        ~TDArray();

        // Copy assignment
        TDArray<T>& operator=(const TDArray<T>& other);
        // Move assignment
        TDArray<T>& operator=(TDArray<T>&& other) noexcept;

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
        void Reserve(size_t capacity);
        void Resize(size_t size, const T& value = T {});

        // Modifiers
        void Clear() noexcept;
        void PushBack(const T& value);
        void PushBack(T&& value);
        template <typename... Args>
        T& EmplaceBack(Args&&... args);
        void PopBack();

        template <typename UnaryPredicate>
        void RemoveIf(UnaryPredicate p);

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
            bool operator==(const Iterator& other) const { return ptr == other.ptr; }

            T& operator*() const { return *ptr; }
            Iterator operator+(int offset) const { return Iterator(ptr + offset); }

        private:
            T* ptr;
        };

        class ConstIterator
        {
        public:
            ConstIterator(T* ptr)
                : ptr(ptr)
            {
            }
            ConstIterator operator++()
            {
                ++ptr;
                return *this;
            }
            bool operator==(const ConstIterator& other) const { return ptr == other.ptr; }
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

		template <typename U>
		struct EqualTo
		{
			constexpr bool operator()(const U& a, const U& b) { return a == b; }
		};

		template <typename CompareFunc = EqualTo<typename RemovePointer<Iterator>::type>>
		void Unique(CompareFunc compare = CompareFunc());

        T*& Data() { return m_Data; }
        const T* Data() const { return m_Data; }

        // Helper function for deleting allocated memory
        void Destroy() noexcept;

    private:
        T* m_Data         = nullptr;
        size_t m_Size     = 0;
        size_t m_Capacity = 0;
        Arena* m_Arena    = nullptr;

        // Helper function for copying elements
        void CopyElements(const TDArray<T>& other);
    };

    // Constructor implementations
    template <class T>
    TDArray<T>::TDArray(Arena* arena)
        : m_Arena(arena)
    {
    }

    template <class T>
    TDArray<T>::TDArray(const TDArray<T>& other)
        : m_Arena(other.m_Arena)
        , m_Size(0)
        , m_Capacity(0)
    {
        CopyElements(other);
    }

    template <class T>
    TDArray<T>::TDArray(TDArray<T>&& other) noexcept
        : m_Data(other.m_Data)
        , m_Size(other.m_Size)
        , m_Capacity(other.m_Capacity)
        , m_Arena(other.m_Arena)
    {
        other.m_Data     = nullptr;
        other.m_Size     = 0;
        other.m_Capacity = 0;
    }

    template <class T>
    TDArray<T>::TDArray(size_t size, const T& initial, Arena* arena)
        : m_Size(0)
        , m_Capacity(0)
        , m_Arena(arena)
    {
        Reserve(size);
        for(size_t i = 0; i < size; ++i)
            m_Data[i] = initial;

        m_Size = size;
    }

    template <class T>
    TDArray<T>::TDArray(std::initializer_list<T> values, Arena* arena)
        : m_Size(values.size())
        , m_Capacity(0)
        , m_Arena(arena)
    {
        Reserve(m_Size);
        size_t index = 0;
        for(auto& value : values)
            m_Data[index++] = value;
    }

    // Destructor implementation
    template <class T>
    TDArray<T>::~TDArray()
    {
        Destroy();
    }

    // Copy assignment implementation
    template <class T>
    TDArray<T>& TDArray<T>::operator=(const TDArray<T>& other)
    {
        if(this != &other)
        {
            Destroy();
            CopyElements(other);
            m_Arena = other.m_Arena;
        }
        return *this;
    }

    // Move assignment implementation
    template <class T>
    TDArray<T>& TDArray<T>::operator=(TDArray<T>&& other) noexcept
    {
        Swap(m_Data, other.m_Data);
        Swap(m_Arena, other.m_Arena);
        Swap(m_Size, other.m_Size);
        Swap(m_Capacity, other.m_Capacity);

        other.m_Capacity = 0;
        other.m_Data     = nullptr;
        other.m_Arena    = nullptr;
        other.m_Size     = 0;
        return *this;
    }

    // Element access implementations
    template <class T>
    T& TDArray<T>::operator[](size_t index)
    {
        ASSERT(index < m_Size, "Index must be less than vector's size");
        return m_Data[index];
    }

    template <class T>
    const T& TDArray<T>::operator[](size_t index) const
    {
        ASSERT(index < m_Size, "Index must be less than vector's size");
        return m_Data[index];
    }

    template <class T>
    T& TDArray<T>::Front()
    {
        return *begin();
    }

    template <class T>
    const T& TDArray<T>::Front() const
    {
        return *begin();
    }

    template <class T>
    T& TDArray<T>::Back()
    {
        return m_Data[m_Size - 1];
    }

    template <class T>
    const T& TDArray<T>::Back() const
    {
        return m_Data[m_Size - 1];
    }

    // Capacity implementations
    template <class T>
    bool TDArray<T>::Empty() const noexcept
    {
        return m_Size == 0;
    }

    template <class T>
    size_t TDArray<T>::Size() const noexcept
    {
        return m_Size;
    }

    template <class T>
    size_t TDArray<T>::Capacity() const noexcept
    {
        return m_Capacity;
    }

    template <class T>
    void TDArray<T>::Reserve(size_t capacity)
    {
        if(capacity <= m_Capacity)
            return;

        T* newData = nullptr;
        if(m_Arena)
            newData = PushArrayNoZero(m_Arena, T, capacity);
        else
            newData = new T[capacity];

        if(m_Data)
        {
            for(size_t i = 0; i < m_Size; ++i)
                newData[i] = Move(m_Data[i]);

            if(!m_Arena)
                delete[] m_Data;
        }

        m_Data     = newData;
        m_Capacity = capacity;
    }

    template <class T>
    void TDArray<T>::Resize(size_t size, const T& value)
    {
        Reserve(size);
        for(size_t i = m_Size; i < size; ++i)
            m_Data[i] = value;
        m_Size = size;
    }

    // Modifiers implementations
    template <class T>
    void TDArray<T>::Clear() noexcept
    {
        for(size_t i = 0; i < m_Size; ++i)
        {
            m_Data[i].~T();
        }

        if(m_Data)
            MemorySet(m_Data, 0, m_Size * sizeof(T));
        m_Size = 0;
    }

    template <class T>
    void TDArray<T>::PushBack(const T& value)
    {
        if(m_Size == m_Capacity)
            Reserve(m_Capacity == 0 ? 1 : m_Capacity * 2);
        m_Data[m_Size++] = value;
    }

    template <class T>
    void TDArray<T>::PushBack(T&& value)
    {
        if(m_Size == m_Capacity)
            Reserve(m_Capacity == 0 ? 1 : m_Capacity * 2);
        m_Data[m_Size++] = value;
    }

    template <class T>
    template <typename... Args>
    T& TDArray<T>::EmplaceBack(Args&&... args)
    {
        if(m_Size == m_Capacity)
            Reserve(m_Capacity == 0 ? 1 : m_Capacity * 2);

        m_Data[m_Size] = T(Forward<Args>(args)...);
        return m_Data[m_Size++];
    }

    template <class T>
    void TDArray<T>::PopBack()
    {
        if(m_Size > 0)
        {
            m_Data[m_Size - 1].~T();
            --m_Size;
        }
    }

    // Helper function implementations
    template <class T>
    void TDArray<T>::CopyElements(const TDArray<T>& other)
    {
        Reserve(other.m_Capacity);
        for(size_t i = 0; i < other.m_Size; ++i)
            m_Data[i] = other.m_Data[i];
        m_Size = other.m_Size;
    }

    template <class T>
    void TDArray<T>::Destroy() noexcept
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

        m_Data     = nullptr;
        m_Size     = 0;
        m_Capacity = 0;
    }

    template <class T>
    template <typename UnaryPredicate>
    void TDArray<T>::RemoveIf(UnaryPredicate p)
    {
        size_t writeIndex = 0;
        for(size_t readIndex = 0; readIndex < m_Size; ++readIndex)
        {
            if(!p(m_Data[readIndex]))
            {
                if(writeIndex != readIndex)
                {
                    m_Data[writeIndex] = Move(m_Data[readIndex]);
                }
                ++writeIndex;
            }
        }
        Resize(writeIndex);
    }

    template <class T>
    template <typename CompareFunc>
    void TDArray<T>::Unique(CompareFunc compare)
    {
        if (m_Size < 2)
            return;

        size_t writeIndex = 1; // Start writing from the second element
        for (size_t readIndex = 1; readIndex < m_Size; ++readIndex)
        {
            // Compare with the last written element
            if (!compare(m_Data[readIndex], m_Data[writeIndex - 1]))
            {
                m_Data[writeIndex] = Move(m_Data[readIndex]);
                ++writeIndex;
            }
        }

        Resize(writeIndex); // Resize the array to the new size
    }
}
