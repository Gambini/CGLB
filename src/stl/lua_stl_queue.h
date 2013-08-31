#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include <queue>
#include "lua_stl.h"

namespace cglb { 
namespace stl {

template<typename T, typename refType >
//Common adapter for vector,dequeue, and list (though not the actual queue itself)
struct expose_generic_queue
{
    //yup, have to remove reference, add const, then add back in the reference
    typedef typename add_constreference<refType>::type constRefType;
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<T>(L,name)
            .template add<refType (T::*)()>("front",&T::front)
            .template add<refType (T::*)()>("back",&T::back)
            .template add<void (T::*)(constRefType)>("push_back",&T::push_back);
    }
};

}
}
