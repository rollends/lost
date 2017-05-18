#include <algorithm>
#include <cstdlib>
#include <thread>

#include "lua.hpp"

extern "C"
{
    #include "lvm.h"
}

#include "lost_job.hpp"
#include "lost_jobproducer.hpp"
#include "lost_scheduler.hpp"

void worker(Scheduler& scheduler);

int main()
{
    Scheduler scheduler;
    JobProducer factory(scheduler);

    size_t const ThreadCount = 3;
    std::vector<std::thread> workers;
    std::generate_n(std::back_inserter(workers), ThreadCount, [&scheduler]() { return std::thread(worker, std::ref(scheduler)); } );

    // Dummy work
    for(int i = 0; i < 300; ++i)
    {
        factory.produceFromFile("test_scripts/test.lua", 1);
    }

    // For every worker, create a kill job.
    for(size_t i = 0; i < ThreadCount; ++i)
    {
        factory.produceFromFile("test_scripts/kill.lua", 0);
    }

    // Wait for workers to be killed
    std::for_each( std::begin(workers),
                   std::end(workers),
                   []( auto& w ) {
                       w.join();
                   });
}

thread_local bool workerShouldLive = true;
int worker_kill( lua_State * )
{
    workerShouldLive = false;
    return 0;
}

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
