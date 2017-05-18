#include "lost_job.hpp"
#include "lost_jobproducer.hpp"

extern "C"
{
    #include "ldo.h"
}

struct rJobProducer
{
    JobProducer& producer;

    void operator () ()
    {
        producer();
    }
};


JobProducer::JobProducer(Scheduler& s, LuaStatePool& pool)
  : shouldLive(true),
    scheduler(s),
    statePool(pool),
    thread( rJobProducer{ *this } )
{
}

JobProducer::~JobProducer()
{
    kill();
}

void JobProducer::kill()
{
    shouldLive = false;

    do
    {
        mutex.lock();
        requestPending.notify_one();
        mutex.unlock();
    } while( queue.size() ); // Keep looping just in case the signal is missed because there were actually items in the queue

    thread.join();
}

void JobProducer::produceFromFile(std::string filePath, int priority)
{
    std::lock_guard<std::mutex> lock(mutex);
    queue.push(JobRequest{filePath, priority, FILE});
    requestPending.notify_one();
}

void JobProducer::operator () ()
{
    struct ExitException{ };

    try
    {
        JobRequest request;

        for(;;)
        {
            {
                mutex.lock();
                if( queue.empty() )
                {
                    requestPending.wait(mutex,
                                        [this](){
                                            if( !queue.empty() )
                                                return true;
                                            if( !shouldLive )
                                                throw ExitException();
                                            return false;
                                        });
                }
                request = queue.front();
                queue.pop();
                mutex.unlock();
            }

            auto state = statePool.takeState();

            luaL_dofile(state, request.file.c_str());
            lua_getglobal(state, "task_main");
            luaD_precall(state, state->top - 1, 0);
            state->ci->callstatus |= CIST_FRESH;

            scheduler.giveJob( LostJob{state, request.priority} );
        }
    }
    catch( ExitException )
    {
        mutex.unlock();
    }
}
