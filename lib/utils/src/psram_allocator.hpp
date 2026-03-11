#pragma once
#include <Arduino.h>
#include <vector>
#include <esp_heap_caps.h>

template <class T>
struct PsramAllocator {
    typedef T value_type;
    PsramAllocator() = default;
    template <class U> constexpr PsramAllocator(const PsramAllocator<U>&) noexcept {}
    [[nodiscard]] T* allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) 
            throw std::bad_alloc();
        
        // Try PSRAM first
        void* p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM);
        
        // Fallback to internal RAM if PSRAM fails or is not available
        if (!p) 
            p = malloc(n * sizeof(T));
        
        if (!p) 
            throw std::bad_alloc();
        return static_cast<T*>(p);
    }
    void deallocate(T* p, std::size_t) noexcept {
        free(p);
    }
};

template <class T, class U>
bool operator==(const PsramAllocator<T>&, const PsramAllocator<U>&) { return true; }
template <class T, class U>
bool operator!=(const PsramAllocator<T>&, const PsramAllocator<U>&) { return false; }
