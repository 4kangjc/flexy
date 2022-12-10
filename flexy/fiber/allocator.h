#pragma once

#include <stdlib.h>
#include <memory>
// #include <memory_resource>

class MallocStackAllocator {
public:
    static void* Alloc(size_t size) { return malloc(size); }
    static void Dealloc(void* vp, size_t size) { return free(vp); }
};

class StdStackAllocator {
public:
    static void* Alloc(size_t size) { return a.allocate(size); }
    static void Dealloc(void* vp, size_t size) {
        a.deallocate((char*)vp, size);
    }

private:
    static std::allocator<char> a;
};

// static std::pmr::polymorphic_allocator<char> a;

// class PolymorphicAllocator {
// public:
//     static void* Alloc(size_t size) {
//         return a.allocate(size);
//     }
//     static void Dealloc(void* vp, size_t size) {
//         a.deallocate((char*)vp, size);
//     }
// private:
//     // static std::pmr::polymorphic_allocator<char>
//     a(std::pmr::synchronized_pool_resource);
// };