#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include "lua_stl.h"
#include <functional>
#include <algorithm>
#include <iterator>
#include <cglb/class_operator_wrapper.h>

namespace cglb {
namespace stl {

template<typename itrType>
struct expose_concept_Iterator
{
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<itrType>(L,name)
            .template opAdd(&std::next<itrType>)
            .template add("get", &class_operator_wrapper::dereference<itrType>);
    }
};

template<typename itrType>
struct expose_concept_InputIterator
{
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<itrType>(L,name)
            .template opEq(&class_operator_wrapper::equal<itrType,itrType>);

        expose_concept_Iterator<itrType>::Expose(L,name);

        //can't implement all of the different overloads of ++
    }
};

template<typename itrType>
struct expose_concept_ForwardIterator
{
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<itrType>(L,name)
            .template constructor<>(); //default constructable

        expose_concept_InputIterator<itrType>::Expose(L,name);

        //can't implement all of the different overloads of ++, again
    }
};

template<typename itrType>
struct expose_concept_BidirectionalIterator
{
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<itrType>(L,name)
            .template opSub(&std::prev<itrType>)
            .template opUnm(&std::prev<itrType>);

        expose_concept_ForwardIterator<itrType>::Expose(L,name);
    }
};


template<typename itrType>
struct expose_concept_RandomAccessIterator
{
    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<itrType>(L,name)
            .template opLe(&class_operator_wrapper::less_equal<itrType,itrType>)
            .template opLt(&class_operator_wrapper::less<itrType,itrType>);
        //also need a [] operator

        expose_concept_BidirectionalIterator<itrType>::Expose(L,name);

        //no overload support yet, but hopefully the previous overloads for add/sub are good enough
    }
};

}
}
