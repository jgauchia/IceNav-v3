/**
 * @file InternalAllocator.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Native ESP-IDF Internal RAM Allocator for STL containers
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once
#include <cstddef>
#include <vector>
#include <new>
#include "esp_heap_caps.h"
#include "esp_log.h"

/**
 * @brief Allocator that forces memory allocation in Internal RAM using ESP-IDF native API.
 *
 * @tparam T Type of elements to allocate
 */
template <class T>
struct InternalAllocator {
    typedef T value_type;
    
    InternalAllocator() = default;
    
    template <class U> 
    constexpr InternalAllocator(const InternalAllocator<U>&) noexcept {}
    
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n == 0) 
            return nullptr;
        if (n > static_cast<std::size_t>(-1) / sizeof(T)) 
            throw std::bad_alloc();
        
        // Use heap_caps_malloc to force allocation in Internal RAM with 8-bit alignment
        void* p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!p) 
        {
            ESP_LOGE("InternalAlloc", "Failed to allocate %u bytes in Internal RAM", (unsigned int)(n * sizeof(T)));
            throw std::bad_alloc();
        }
        return static_cast<T*>(p);
    }
    
    void deallocate(T* p, std::size_t) noexcept {
        heap_caps_free(p);
    }
};

template <class T, class U>
bool operator==(const InternalAllocator<T>&, const InternalAllocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const InternalAllocator<T>&, const InternalAllocator<U>&) { return false; }
