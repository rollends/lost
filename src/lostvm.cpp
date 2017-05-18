#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

#include "lost_allocator.hpp"
#include "lost_jobproducer.hpp"
#include "lost_luastatepool.hpp"
#include "lost_scheduler.hpp"
#include "lost_workerpool.hpp"

int main()
{
    for(int i = 0; i < LUA_NUMTAGS; ++i)
    {
        AllocCount[i].store(0);
        AllocSize[i].store(0);
    }

    Scheduler scheduler;
    LuaStatePool statePool(50);
    JobProducer factory(scheduler, statePool);

    {
        WorkerPool workers(scheduler, factory, statePool);

        // Dummy work
        for(int i = 0; i < 30000; ++i)
        {
            factory.produceFromFile("test_scripts/test.lua", 2);
        }
    }

    // Workers died. Check memory usage :)
    for(int i = 0; i < LUA_NUMTAGS; ++i)
    {
        std::cout   << std::setw(2) << i << " : "
                    << std::setw(5) << AllocCount[i].load() << " : "
                    << std::setw(12) << AllocSize[i].load() << std::endl;
    }

    std::exit(0);
}
