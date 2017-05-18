#include "lua.hpp"

#include "lost_allocator.hpp"
#include "lost_library.hpp"
#include "lost_luastatepool.hpp"

int worker_kill( lua_State * );
int worker_newjob( lua_State * state );

namespace
{
    std::array<luaL_Reg, 6> permittedLibraries
    {
        luaL_Reg{"_G", luaopen_base}
      , luaL_Reg{LUA_COLIBNAME, luaopen_coroutine}
      , luaL_Reg{LUA_TABLIBNAME, luaopen_table}
      , luaL_Reg{LUA_STRLIBNAME, luaopen_string}
      , luaL_Reg{LUA_MATHLIBNAME, luaopen_math}
      , luaL_Reg{LUA_UTF8LIBNAME, luaopen_utf8}
    };
}

LuaStatePool::LuaStatePool(size_t poolSize)
  : statePool(poolSize),
    indEmptySlot(0),
    indFullSlot(0),
    available(poolSize)
{
    auto libWorker = makeLibrary("worker", "kill", worker_kill, "new_job", worker_newjob);

    for(size_t i = 0; i < statePool.capacity(); ++i)
    {
        lua_State* state = statePool[i] = lua_newstate(lost_debug_frealloc, NULL);

        // Open libraries for the state
        for (auto&& library : permittedLibraries)
        {
            luaL_requiref(state, library.name, library.func, 1);
            lua_pop(state, 1);
        }

        libWorker( state );
    }
}

LuaStatePool::~LuaStatePool()
{
    for(auto&& state : statePool)
        lua_close(state);
}

void LuaStatePool::giveState(lua_State* state)
{
    std::unique_lock<std::mutex> door(mutex);
    statePool[indEmptySlot] = state;
    indEmptySlot = (indEmptySlot + 1) % statePool.size();
    available++;
    freeState.notify_one();
}

lua_State* LuaStatePool::takeState()
{
    std::unique_lock<std::mutex> door(mutex);
    if( available == 0 )
    {
        freeState.wait( door,
                        [this]() {
                            return available > 0;
                        } );
    }
    auto result = statePool[indFullSlot];
    indFullSlot = (indFullSlot + 1) % statePool.size();
    available--;
    return result;
}
