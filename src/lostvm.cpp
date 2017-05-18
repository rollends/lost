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
#include "lost_workerpool.hpp"

int main()
{
    Scheduler scheduler;
    JobProducer factory(scheduler);

    {
        WorkerPool workers(scheduler, factory);

        // Dummy work
        for(int i = 0; i < 300; ++i)
        {
            factory.produceFromFile("test_scripts/test.lua", 1);
        }
    }
}
