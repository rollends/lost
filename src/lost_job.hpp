#ifndef LOST_JOB_HPP
#define LOST_JOB_HPP

#include "lua.hpp"

struct LostJob
{
    lua_State* state;
    int priority;

    bool operator < (LostJob const & other) const;
};

#endif
