#ifndef LUA_STATE_POOL
#define LUA_STATE_POOL

#include <condition_variable>
#include <mutex>
#include <vector>

#include "lua.hpp"

class LuaStatePool
{
public:
    LuaStatePool(size_t poolSize);
    ~LuaStatePool();

    void giveState(lua_State* state);
    lua_State* takeState();

private:
    std::vector<lua_State*> statePool;
    size_t indEmptySlot;
    size_t indFullSlot;
    size_t available;
    std::mutex mutex;
    std::condition_variable freeState;
};

#endif
