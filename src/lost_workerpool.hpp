#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include <thread>
#include <vector>

#include "lost_jobproducer.hpp"
#include "lost_luastatepool.hpp"
#include "lost_scheduler.hpp"

class WorkerPool
{
public:
    WorkerPool(Scheduler&, JobProducer&, LuaStatePool&);
    ~WorkerPool();

private:
    Scheduler& scheduler;
    JobProducer& factory;
    LuaStatePool& statePool;
    std::vector<std::thread> workers;

    static size_t constexpr InitialThreadCount = 3;
};

#endif
