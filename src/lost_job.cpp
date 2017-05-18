#include "lost_job.hpp"

bool LostJob::operator < (LostJob const & other) const
{
    return priority < other.priority;
}
