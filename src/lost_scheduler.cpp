#include "lost_scheduler.hpp"

void Scheduler::giveJob(LostJob const &job)
{
    std::unique_lock<std::mutex> guard(mutex);
    queue.push(job);
    workPending.notify_one();
}

LostJob Scheduler::takeJob()
{
    std::unique_lock<std::mutex> door(mutex);

    if(queue.empty())
    {
        workPending.wait(door, [this](){ return 0 != queue.size(); });
    }

    LostJob job = queue.top();
    queue.pop();

    return job;
}
