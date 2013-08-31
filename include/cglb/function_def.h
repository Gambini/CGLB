#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include "function_traits.h"
#include "class_luarep.h"
#include "luafn_interop.h"
#include "policy/policy.h"
#include "lua_include.h"
#include <type_traits>
#include <typeinfo>

namespace cglb {
    
template <typename traits, typename FnPtrT, typename policy>
struct function_def
{
    function_def(FnPtrT fp) : fnptr(fp)
    {
    }
    function_def() : fnptr(nullptr)
    {
    }

    FnPtrT fnptr;

    /**
      * Used as the C callback associated to the function.
      * The upvalue at index 1 is an instance of this class
      */
    static int LuaFunction(lua_State* L)
    {
        typedef function_def<traits,FnPtrT,policy> ThisT;
        ThisT* self = (ThisT*)lua_touserdata(L,lua_upvalueindex(1));
        
        return detail::GatherArgs<traits::arity + 1>::template Gather<FnPtrT,traits,policy>(L,self->fnptr);
    }

};



template<typename T, typename policy, typename FnPtrT>
struct custom_function_def;

template<typename T, typename policy>
struct custom_function_def<T,policy,int (*)(lua_State*,T*)>
{
    //A function taking a lua_State* and T*, where T* is obtained from
    //index 1 of the stack
    typedef int (*StrictFnPtrT)(lua_State*,T*);
    custom_function_def() : fnptr(nullptr)
    {
    }

    void SetFnPtr(StrictFnPtrT fp)
    {
        fnptr = fp;
    }

    StrictFnPtrT fnptr;

    static int LuaFunction(lua_State* L)
    {
        typedef custom_function_def<T,policy,StrictFnPtrT> ThisT;
        ThisT* self = (ThisT*)lua_touserdata(L,lua_upvalueindex(1));
        T* obj = class_luarep<T>::check(L,1);
        return (*(self->fnptr))(L,obj);
    }
};



template<typename T, typename policy>
struct custom_function_def<T,policy,int (*)(lua_State*)>
{
    //A function taking only a lua_State*
    typedef int (*StrictFnPtr)(lua_State*);
    custom_function_def() : fnptr(nullptr)
    {
    }

    void SetFnPtr(StrictFnPtr fp)
    {
        fnptr = fp;
    }

    StrictFnPtr fnptr;

    static int LuaFunction(lua_State* L)
    {
        typedef custom_function_def<T,policy,StrictFnPtr> ThisT;
        ThisT* self = (ThisT*)lua_touserdata(L,lua_upvalueindex(1));
        return (*(self->fnptr))(L);
    }
};


}
