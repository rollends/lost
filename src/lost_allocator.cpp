#include <atomic>
#include <cstdlib>
#include "lua.hpp"

std::atomic_size_t AllocCount[LUA_NUMTAGS];
std::atomic_size_t AllocSize[LUA_NUMTAGS];

void* lost_debug_frealloc(void * ud, void * ptr, size_t oldSize, size_t newSize)
{
    (void)ud;

    if(newSize && (oldSize < LUA_NUMTAGS))
    {
        AllocCount[oldSize]++;
        AllocSize[oldSize]+=newSize;
    }

    if (newSize == 0)
    {
        free(ptr);
        return NULL;
    }
    else
        return realloc(ptr, newSize);
}
