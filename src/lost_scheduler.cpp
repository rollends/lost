#include "lost_scheduler.hpp"

void Scheduler::giveJob(LostJob const &job)
{
    std::lock_guard<std::mutex> guard(mutex);
    queue.push(job);
    workPending.notify_one();
}

LostJob Scheduler::takeJob()
{
    mutex.lock();

    if(queue.empty())
    {
        workPending.wait(mutex, [this](){ return 0 != queue.size(); });
    }

    LostJob job = queue.top();
    queue.pop();

    mutex.unlock();

    return job;
}
