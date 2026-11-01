#pragma once
// sol2 needs these to treat engine smart pointers as owning handles.
// Include in every TU that passes SharedPtr/UniquePtr/WeakPtr through sol.
#include <sol/sol.hpp>

#ifdef CUSTOM_SMART_PTR
namespace sol
{
    template <typename T>
    struct unique_usertype_traits<Lumos::SharedPtr<T>>
    {
        typedef T type;
        typedef Lumos::SharedPtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };

    template <typename T>
    struct unique_usertype_traits<Lumos::UniquePtr<T>>
    {
        typedef T type;
        typedef Lumos::UniquePtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };

    template <typename T>
    struct unique_usertype_traits<Lumos::WeakPtr<T>>
    {
        typedef T type;
        typedef Lumos::WeakPtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type& ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type& ptr)
        {
            return ptr.get();
        }
    };
}

#endif
