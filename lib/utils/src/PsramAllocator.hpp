/**
 * @file PsramAllocator.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Native ESP-IDF PSRAM Allocator for STL containers
 * @version 0.2.4
 * @date 2026-01
 */

#pragma once
#include <cstddef>
#include <vector>
#include <new>
#include "esp_heap_caps.h"
#include "esp_log.h"

/**
 * @brief Allocator that forces memory allocation in PSRAM (SPIRAM) using ESP-IDF native API.
 * @tparam T Type of elements to allocate
 */
template <class T>
struct PsramAllocator {
    typedef T value_type;
    
    PsramAllocator() = default;
    
    template <class U> 
    constexpr PsramAllocator(const PsramAllocator<U>&) noexcept {}
    
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > static_cast<std::size_t>(-1) / sizeof(T)) throw std::bad_alloc();
        
        // Use heap_caps_malloc to force allocation in SPIRAM with 8-bit alignment
        void* p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!p) {
            ESP_LOGE("PsramAlloc", "Failed to allocate %u bytes in PSRAM", (unsigned int)(n * sizeof(T)));
            throw std::bad_alloc();
        }
        return static_cast<T*>(p);
    }
    
    void deallocate(T* p, std::size_t) noexcept {
        heap_caps_free(p);
    }
};

template <class T, class U>
bool operator==(const PsramAllocator<T>&, const PsramAllocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const PsramAllocator<T>&, const PsramAllocator<U>&) { return false; }
