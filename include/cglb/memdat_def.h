#pragma once
#include "lua_include.h"
#include "class_luarep.h"
#include "luafn_interop.h"
#include "policy/policy.h"

/**
  * Similar to the function_traits.h, this is needed to
  * extract the type out of a member data pointer.
  */
template <typename DatT>
struct memdat_traits
{
    typedef DatT data_type;
};

template<typename ClassType, typename DatType>
struct memdat_traits<DatType ClassType::*> : public memdat_traits<DatType>
{
    typedef ClassType owner_type;
};

namespace cglb {

/**
 * For storing a pointer-to-member of a class
 */
template <typename MemPtrT, typename traits>
struct memdat_def
{
    memdat_def(MemPtrT mp) : memptr(mp)
    {
    }
    memdat_def() : memptr(nullptr)
    {
    }
    MemPtrT memptr;

    
    static int LuaGetFunction(lua_State* L)
    {
        /*
         * __cglb_getters is set up in a way so that index 1
         * is the pointer to the instance of T, and index 2 is
         * the key (which isn't used).
         * upvalue at index 1 is an instance of this class
         */
        typedef typename traits::owner_type OwnerT;
        typedef policy<return_gc<std::false_type>> pol; //get should never return a gc type
        typedef memdat_def<MemPtrT,traits> ThisT;
        ThisT* self = (ThisT*)lua_touserdata(L,lua_upvalueindex(1));
        OwnerT* obj = class_luarep<OwnerT>::check(L,1);
        if(!obj)
        {
            lua_pushnil(L);
            return 1;
        }
        typename traits::data_type ret = obj->*(self->memptr);
        //FIXME: Cannot have a gc'd get function
        detail::PushFuncResult<typename traits::data_type,pol>(L,ret);
        return 1;
    }

    
    static int LuaSetFunction(lua_State* L)
    {
        /*
            __cglb_setters is set up in a way so that 
            index 1 is the instance of T
            index 2 is the key (not used here)
            index 3 is the value
            upvalue index 1 is an instance of this class
        */
        typedef typename traits::owner_type OwnerT;
        typedef memdat_def<MemPtrT,traits> ThisT;
        ThisT* self = (ThisT*)lua_touserdata(L,lua_upvalueindex(1));

        OwnerT* obj = class_luarep<OwnerT>::check(L,1);
        if(!obj)
        {
            lua_pushnil(L);
            return 0;
        }

        typename traits::data_type val = detail::GetFuncArg<typename traits::data_type>(L,3);
        obj->*(self->memptr) = val;
        return 0;
    }
};

}
