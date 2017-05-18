#ifndef WORKER_POOL_HPP
#define WORKER_POOL_HPP

#include <thread>
#include <vector>

#include "lost_scheduler.hpp"
#include "lost_jobproducer.hpp"

class WorkerPool
{
public:
    WorkerPool(Scheduler&, JobProducer&);
    ~WorkerPool();

private:
    Scheduler& scheduler;
    JobProducer& factory;
    std::vector<std::thread> workers;

    static size_t constexpr InitialThreadCount = 3;
};

#endif
