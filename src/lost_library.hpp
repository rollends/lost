#ifndef LOST_LIBRARY_HPP
#define LOST_LIBRARY_HPP

#include <type_traits>

#include "lua.hpp"

template < typename... >
struct LostLibraryFunction
{ };

template< typename ThisFunction >
struct LostLibraryFunction< ThisFunction >
{
    using FuncType = typename std::enable_if_t< std::is_same<ThisFunction, int (*)(lua_State*) >::value, ThisFunction >;

    void operator () (lua_State * state)
    {
        lua_pushcfunction(state, fx);
        lua_setfield(state, lua_gettop(state) - 1, name);
    }

    LostLibraryFunction(char const * name, FuncType fx)
      : name(name),
        fx(fx)
    { }

private:
    char const * name;
    ThisFunction fx;
};

template< typename ThisFunction,
          typename NameType,
          typename... OtherFunctions >
struct LostLibraryFunction< ThisFunction, NameType, OtherFunctions... >
{
    using FuncType = typename std::enable_if_t< std::is_same<ThisFunction, int (*)(lua_State*) >::value, ThisFunction >;

    void operator () (lua_State * state)
    {
        lua_pushcfunction(state, fx);
        lua_setfield(state, lua_gettop(state) - 1, name);
        functionList(state);
    }

    LostLibraryFunction(char const * name, FuncType fx, char const * nextName, OtherFunctions... others)
      : name(name),
        fx(fx),
        functionList(nextName, others...)
    { }

private:
    char const * name;
    ThisFunction fx;
    LostLibraryFunction<OtherFunctions...> functionList;
};

template< typename... FunctionTypes >
struct LostLibrary
{
    LostLibrary(const char * name, const char * fname, FunctionTypes... functions )
      : name(name),
        functionList(fname, functions...)
    { }

    void operator () (lua_State * state)
    {
        lua_createtable(state, 0, 1);
        functionList(state);
        lua_setglobal(state, name);
    }

private:
    const char * name;
    LostLibraryFunction<FunctionTypes...> functionList;
};

template<typename... FunctionTypes>
static LostLibrary<FunctionTypes...> makeLibrary(char const * name, char const * fname, FunctionTypes... functions)
{
    return LostLibrary<FunctionTypes...>(name, fname, functions...);
}

#endif
