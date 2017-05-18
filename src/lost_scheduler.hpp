#ifndef LOST_SCHEDULER_HPP
#define LOST_SCHEDULER_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

#include "lost_job.hpp"

class Scheduler
{
public:
    void giveJob(LostJob const &);
    LostJob takeJob();

private:
    std::priority_queue<LostJob> queue;
    std::mutex mutex;
    std::condition_variable_any workPending;
};

#endif
