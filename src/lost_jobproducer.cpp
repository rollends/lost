#include <algorithm>
#include <cstring>
#include <unordered_map>

#include "lost_job.hpp"
#include "lost_jobproducer.hpp"
#include <cassert>

extern "C"
{
    #include "ldo.h"
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

    int write2hexbuffer(lua_State *, const void *p, size_t sz, void* ud)
    {
        const char * data = (const char *)p;
        size_t indNibble = 0;

        std::generate_n( std::back_inserter(*(std::string*)ud),
                         2 * sz,
                         [&indNibble, data]() {
                             auto ind = indNibble++;    // This assumes generator is called EXACTLY once for every iteration.
                             return 'A' + (0xF & (data[ind / 2] >> (4*(1 - ind % 2))));
                         } );
        return 0;
    }

    const char * readhexbuffer(lua_State *, void *data, size_t *size)
    {
        std::string* wb = (std::string*)data;
        if(*wb != "")
        {
            char * mem = new char[wb->length() / 2];
            size_t ind = 0;
            *size = wb->length() / 2;
            std::generate( mem,
                           mem + *size,
                           [&ind, wb]() {
                               auto i = ind++;
                               char value = 0;
                               value = ((*wb)[i] - 'A') << 4;
                               i = ind++;
                               value |= ((*wb)[i] - 'A');
                               return value;
                           });
            *wb = "";
            return mem;
        }
        return NULL;
    }

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
    } while( !queue.empty() ); // Keep looping just in case the signal is missed because there were actually items in the queue

    thread.join();
}

void JobProducer::produceFromFile(std::string filePath, int priority)
{
    std::unique_lock<std::mutex> door(mutex);
    queue.push(JobRequest{filePath, priority, FILE});
    requestPending.notify_one();
}

void JobProducer::produceFromStack(lua_State * state, int priority)
{
    std::string buffer;
    if( lua_dump(state, write2hexbuffer, &buffer, 1) == 0 )
    {
        std::unique_lock<std::mutex> door(mutex);
        queue.push(JobRequest{buffer, priority, BYTECODE});
        requestPending.notify_one();
    }
}

void JobProducer::rJobProducer::operator () ()
{
    producer();
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
            switch(request.type)
            {
            case FILE:
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
                lua_pcall(state, 0, LUA_MULTRET, 0);
                lua_getglobal(state, "task_main");
                break;

            case BYTECODE:
                lua_load(state, readhexbuffer, &request.file, request.file.c_str(), "b");
            }

            luaD_precall(state, state->top - 1, 0);
            state->ci->callstatus |= CIST_FRESH;

            scheduler.giveJob( LostJob{state, request.priority} );
        }
    }
    catch( ExitException ) { }
}
