/**
 * @file PsramAllocator.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Memory allocators for PSRAM and Internal SRAM
 * @version 0.3.0
 * @date 2026-09
 */

#pragma once

#include <cstddef>
#include <memory>
#include "esp_heap_caps.h"

/**
 * @struct PsramAllocator
 * @brief Allocator that forces memory into SPIRAM (PSRAM)
 */
template <class T>
struct PsramAllocator
{
    typedef T value_type;

    PsramAllocator() = default;

    template <class U>
    PsramAllocator(const PsramAllocator<U>&) {}

    T* allocate(std::size_t n)
    {
        void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t)
    {
        heap_caps_free(p);
    }
};

template <class T, class U>
bool operator==(const PsramAllocator<T>&, const PsramAllocator<U>&)
{
    return true;
}

template <class T, class U>
bool operator!=(const PsramAllocator<T>&, const PsramAllocator<U>&)
{
    return false;
}

/**
 * @struct InternalRamAllocator
 * @brief Allocator that forces memory into Internal SRAM for performance
 */
template <class T>
struct InternalRamAllocator
{
    typedef T value_type;

    InternalRamAllocator() = default;

    template <class U>
    InternalRamAllocator(const InternalRamAllocator<U>&) {}

    T* allocate(std::size_t n)
    {
        return static_cast<T*>(heap_caps_malloc(n * sizeof(T), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }

    void deallocate(T* p, std::size_t)
    {
        heap_caps_free(p);
    }
};

template <class T, class U>
bool operator==(const InternalRamAllocator<T>&, const InternalRamAllocator<U>&)
{
    return true;
}

template <class T, class U>
bool operator!=(const InternalRamAllocator<T>&, const InternalRamAllocator<U>&)
{
    return false;
}

/**
 * @brief Allocator that prefers Internal SRAM and falls back to PSRAM
 */
template <class T>
struct InternalFirstAllocator
{
    typedef T value_type;

    InternalFirstAllocator() = default;

    template <class U>
    InternalFirstAllocator(const InternalFirstAllocator<U>&) {}

    T* allocate(std::size_t n)
    {
        void* ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (ptr == nullptr)
            ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (ptr == nullptr)
            throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t)
    {
        heap_caps_free(p);
    }
};

template <class T, class U>
bool operator==(const InternalFirstAllocator<T>&, const InternalFirstAllocator<U>&)
{
    return true;
}

template <class T, class U>
bool operator!=(const InternalFirstAllocator<T>&, const InternalFirstAllocator<U>&)
{
    return false;
}
