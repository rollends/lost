#ifndef JOB_PRODUCER_HPP
#define JOB_PRODUCER_HPP

#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>

#include "lua.hpp"

#include "lost_luastatepool.hpp"
#include "lost_scheduler.hpp"

class JobProducer
{
public:
    JobProducer(Scheduler&, LuaStatePool&);
    ~JobProducer();

    void produceFromFile(std::string filePath, int priority);
    void produceFromStack(lua_State * state, int priority);

private:
    void kill();
    void operator()();

    enum RequestType { FILE, BYTECODE };

    struct JobRequest
    {
        std::string file;
        int priority;
        RequestType type;
    };

    struct rJobProducer
    {
        JobProducer& producer;
        void operator () ();
    };

    volatile bool shouldLive;

    std::queue<JobRequest> queue;
    std::mutex mutex;
    std::condition_variable requestPending;

    Scheduler& scheduler;
    LuaStatePool& statePool;

    std::thread thread;
};

#endif
