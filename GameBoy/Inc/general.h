#pragma once

#include "typedefs.h"

#include <memory>
#include <cstring>
#include <algorithm>

namespace gb
{
    template <typename T>
    using unique_ptr = std::unique_ptr<T>;

    template <typename T>
    unique_ptr<T[]> make_array(size_t size)
    {
        return unique_ptr<T[]>(new T[size]());
    }

    using std::memcpy;
    using std::copy;

    template <typename... Args>
    void print(const char *, Args&&...)
    {
    }

#define GB_BREAK() do { while (1) {} } while (0)

    f_inline void assert_impl(bool condition,
                              const char *expression,
                              const char *file,
                              int line)
    {
        if (!condition)
        {
            GB_BREAK();
        }
    }

    #ifndef NDEBUG
    #define gb_assert(expression) \
        gb::assert_impl((expression), #expression, __FILE__, __LINE__)
    #else
    #define gb_assert(expression) ((void)0)
    #endif
}
