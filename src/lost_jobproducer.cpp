#include <cstring>
#include <unordered_map>

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

namespace
{
    struct WriteBuffer
    {
        int8_t buffer[1024];
        size_t amount;
    };
    struct ReadBuffer
    {
        int8_t* buffer;
        size_t amount;
    };

    int write2buffer(lua_State *, const void* p, size_t sz, void* ud)
    {
        WriteBuffer* wb = (WriteBuffer*)ud;

        if(wb->amount + sz > 1024) return 1;

        memcpy(wb->buffer + wb->amount, p, sz / sizeof(int8_t));
        wb->amount += sz;
        return 0;
    }

    const char * readbuffer(lua_State *, void *data, size_t *size)
    {
        ReadBuffer* wb = (ReadBuffer*)data;
        if(wb->amount)
        {
            *size = (sizeof(int8_t)*wb->amount) / sizeof(char);
            wb->amount = 0;
            return (const char *)wb->buffer;
        }
        return NULL;
    }
}

void JobProducer::operator () ()
{
    struct ExitException{ };

    WriteBuffer writebuff;
    std::unordered_map< std::string, ReadBuffer > codeCache;

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

            // Load file
            if(codeCache.find(request.file) == codeCache.end())
            {
                writebuff.amount = 0;
                luaL_loadfile(state, request.file.c_str());
                if( lua_dump(state, write2buffer, &writebuff, 0) == 0 )
                {
                    auto code = new int8_t[writebuff.amount];
                    memcpy(code, writebuff.buffer, writebuff.amount);
                    codeCache[request.file] = ReadBuffer{code, writebuff.amount};
                }
            }
            else
            {
                auto buffer = codeCache[request.file];
                lua_load(state, readbuffer, &buffer, request.file.c_str(), "b");
            }

            // Call file
            lua_pcall(state, 0, LUA_MULTRET, 0);
            lua_getglobal(state, "task_main");
            luaD_precall(state, state->top - 1, 0);
            state->ci->callstatus |= CIST_FRESH;

            scheduler.giveJob( LostJob{state, request.priority} );
        }
    }
    catch( ExitException ) { }
}
