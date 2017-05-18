#include <algorithm>

#include "lua.hpp"
extern "C"
{
    #include "lvm.h"
}

#include "lost_workerpool.hpp"

namespace
{
    thread_local bool workerShouldLive = true;
    JobProducer* gJobFactory;
    void worker(Scheduler& scheduler, LuaStatePool& statePool)
    {
        LostJob job;
        while(workerShouldLive)
        {
            job = scheduler.takeJob();

            switch(luaV_custom_execute(job.state, 50))
            {
            case LUA_YIELD:
                scheduler.giveJob(job);
                break;

            default:
                statePool.giveState(job.state);
                break;
            }
        }
    }
}

int worker_kill( lua_State * )
{
    workerShouldLive = false;
    return 0;
}

int worker_newjob( lua_State * state )
{
    auto const top = lua_gettop(state);
    if(top != 2)
    {
        lua_pushliteral(state, "incorrect number of arguments");
        lua_error(state);
    }
    else if(!lua_isinteger(state, lua_gettop(state)))
    {
        lua_pushliteral(state, "priority must be an integral value");
        lua_error(state);
    }
    else if(!lua_isfunction(state, lua_gettop(state) - 1))
    {
        lua_pushliteral(state, "job must be a function type");
        lua_error(state);
    }

    auto priority = lua_tointeger(state, top);

    if( priority < 0 )
    {
        lua_pushliteral(state, "priority must be a positive integer");
        lua_error(state);
    }

    lua_pop(state, 1);

    gJobFactory->produceFromStack(state, priority);

    return 0;
}

WorkerPool::WorkerPool(Scheduler& scheduler, JobProducer& factory, LuaStatePool& pool)
  : scheduler(scheduler),
    factory(factory),
    statePool(pool)
{
    gJobFactory = &factory;
    std::generate_n( std::back_inserter(workers),
                     InitialThreadCount,
                     [&scheduler, &pool]() {
                         return std::thread(worker, std::ref(scheduler), std::ref(pool));
                     } );
}

WorkerPool::~WorkerPool()
{
    for(size_t i = 0; i < workers.size(); ++i)
    {
        factory.produceFromFile("test_scripts/kill.lua", 0);
    }

    std::for_each( std::begin(workers),
                   std::end(workers),
                   []( auto& w ) {
                       w.join();
                   });
}
