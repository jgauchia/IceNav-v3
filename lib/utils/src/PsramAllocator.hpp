/**
 * @file PsramAllocator.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  PSRAM Allocator for STL containers (falls back to Internal if no PSRAM)
 * @version 0.1.0
 * @date 2026-02
 */

#pragma once
#include <cstddef>
#include <new>
#include "esp_heap_caps.h"
#include "esp_log.h"

template <class T>
struct PsramAllocator {
    typedef T value_type;
    PsramAllocator() = default;
    template <class U> 
    constexpr PsramAllocator(const PsramAllocator<U>&) noexcept {}
    
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > static_cast<std::size_t>(-1) / sizeof(T)) throw std::bad_alloc();
        
        // Try SPIRAM first, fallback to Internal 8BIT if not available or failed
        void* p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!p) {
            p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_8BIT);
        }
        if (!p) {
            ESP_LOGE("PsramAlloc", "Failed to allocate %u bytes", (unsigned int)(n * sizeof(T)));
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