#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "lost_allocator.hpp"
#include "lost_jobproducer.hpp"
#include "lost_luastatepool.hpp"
#include "lost_scheduler.hpp"
#include "lost_workerpool.hpp"

int main()
{
    // Workers died. Check memory usage :)
    for(int i = 0; i < LUA_NUMTAGS; ++i)
        AllocCount[i].store(0);

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

    // Workers died. Check memory usage :)
    for(int i = 0; i < LUA_NUMTAGS; ++i)
        std::cout << i << " : " << AllocCount[i].load() << std::endl;
}
