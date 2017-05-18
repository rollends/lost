#include <algorithm>
#include <cstdlib>
#include <thread>

#include "lost_jobproducer.hpp"
#include "lost_luastatepool.hpp"
#include "lost_scheduler.hpp"
#include "lost_workerpool.hpp"

int main()
{
    Scheduler scheduler;
    LuaStatePool statePool(50);
    JobProducer factory(scheduler, statePool);

    {
        WorkerPool workers(scheduler, factory, statePool);

        // Dummy work
        for(int i = 0; i < 300; ++i)
        {
            factory.produceFromFile("test_scripts/test.lua", 1);
        }
    }
}
