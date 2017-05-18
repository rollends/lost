#include "lost_job.hpp"
#include "lost_jobproducer.hpp"

extern "C"
{
    #include "ldo.h"
}

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
        std::unique_lock<std::mutex> door(mutex);
        requestPending.notify_one();
    } while( queue.size() ); // Keep looping just in case the signal is missed because there were actually items in the queue

    thread.join();
}

void JobProducer::produceFromFile(std::string filePath, int priority)
{
    std::unique_lock<std::mutex> door(mutex);
    queue.push(JobRequest{filePath, priority, FILE});
    requestPending.notify_one();
}

void JobProducer::rJobProducer::operator () ()
{
    producer();
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
                std::unique_lock<std::mutex> door(mutex);
                if( queue.empty() )
                {
                    requestPending.wait(door,
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
            }

            auto state = statePool.takeState();

            luaL_dofile(state, request.file.c_str());
            lua_getglobal(state, "task_main");
            luaD_precall(state, state->top - 1, 0);
            state->ci->callstatus |= CIST_FRESH;

            scheduler.giveJob( LostJob{state, request.priority} );
        }
    }
    catch( ExitException ) { }
}
