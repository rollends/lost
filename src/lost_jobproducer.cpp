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


JobProducer::JobProducer(Scheduler& s)
  : shouldLive(true),
    scheduler(s),
    thread( rJobProducer{ *this } )
{
}

JobProducer::~JobProducer()
{
    kill();
}

int worker_kill( lua_State * );

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

    std::array<luaL_Reg, 6> permittedLibraries
    {
        luaL_Reg{"_G", luaopen_base}
      , luaL_Reg{LUA_COLIBNAME, luaopen_coroutine}
      , luaL_Reg{LUA_TABLIBNAME, luaopen_table}
      , luaL_Reg{LUA_STRLIBNAME, luaopen_string}
      , luaL_Reg{LUA_MATHLIBNAME, luaopen_math}
      , luaL_Reg{LUA_UTF8LIBNAME, luaopen_utf8}
    };

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

            auto state = luaL_newstate();

            // Open libraries for the state
            for (auto&& library : permittedLibraries)
            {
                luaL_requiref(state, library.name, library.func, 1);
                lua_pop(state, 1);
            }

            lua_createtable(state, 0, 1);
            lua_pushcfunction(state, worker_kill);
            lua_setfield(state, lua_gettop(state) - 1, "kill");
            lua_setglobal(state, "worker");

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
