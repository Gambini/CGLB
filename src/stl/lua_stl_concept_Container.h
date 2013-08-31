#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include "lua_stl.h"

namespace cglb {
namespace stl {

//T here is the fully qualified name, like std::vector<float>, and it should be made
//from a more specific type
template<typename T, typename itrtype>
//this will export the shared interface between all of the stl containers
struct expose_concept_Container
{
    //for begin(),end()
    typedef itrtype (T::*itr_fnptr)();

    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<T>(L,name)
            .template constructor<>()
            .template add("size",&T::size)
            .template add<itr_fnptr>("begin",&T::begin)
            .template add<itr_fnptr>("iend",&T::end)
            .template add("max_size",&T::max_size)
            .template add("empty",&T::empty);
    }
};


//where T is the fully qualified type name, like std::vector<float>
template<typename T, typename itrType, typename refType, typename ptrType>
struct expose_concept_SequenceContainer
{
    //yup, have to remove reference, add const, then add back in the reference
    typedef typename add_constreference<refType>::type constRefType;

    static void Expose(lua_State* L, const char* name)
    {
        typedef typename std::remove_const<itrType>::type RmConstT;
        class_luadef<T>(L,name)
            .template add("clear",&T::clear)
            .template add<void (T::*)(typename T::size_type, constRefType )>
                ("assign",&T::assign)
            .template add<RmConstT (T::*)(itrType,constRefType)>("insert", &T::insert)
            .template add<RmConstT (T::*)(itrType)>("erase",&T::erase)
            .template add<RmConstT (T::*)(itrType,constRefType)>("emplace", (&T::emplace))
            .template add("clear",&T::clear);
    }
};



template<typename T,typename itrType
        , typename revItrType = typename std::conditional<std::is_const<itrType>::value,
                                typename T::const_reverse_iterator,
                                typename T::reverse_iterator>::type
    >
struct expose_concept_ReversibleContainer
{
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<T>(L,name)
            .template add<revItrType (T::*)()>("rbegin",&T::rbegin)
            .template add<revItrType (T::*)()>("rend",&T::rend);
    }
};

}
}
