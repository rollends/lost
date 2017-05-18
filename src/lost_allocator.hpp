#ifndef LOST_ALLOCATOR_HPP
#define LOST_ALLOCATOR_HPP

#include <atomic>

#include "lua.hpp"

extern std::atomic_size_t AllocCount[LUA_NUMTAGS];

void* lost_debug_frealloc(void * ud, void * ptr, size_t oldSize, size_t newSize);

#endif
