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
    void worker(Scheduler& scheduler)
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
                lua_close(job.state);
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

WorkerPool::WorkerPool(Scheduler& scheduler, JobProducer& factory)
  : scheduler(scheduler),
    factory(factory)
{
    std::generate_n( std::back_inserter(workers),
                     InitialThreadCount,
                     [&scheduler]() {
                         return std::thread(worker, std::ref(scheduler));
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
