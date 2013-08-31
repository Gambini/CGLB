#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include "lua_stl.h"
#include "lua_stl_queue.h"
#include "lua_stl_concept_Container.h"
#include "lua_stl_concept_Iterator.h"
#include <vector>


namespace cglb {
namespace stl {

//where T is the vector<T> and itrtype is vector<T>::iterator or vector<T>::const_iterator
//the other template arguments have the same const-ness as the iterator
template<typename T, 
    typename itrtype,
    typename refType = typename std::conditional<std::is_const<itrtype>::value,
                    typename std::vector<T>::const_reference,
                    typename std::vector<T>::reference>::type,
    typename ptrType = typename std::conditional<std::is_const<itrtype>::value,
                    typename std::vector<T>::const_pointer,
                    typename std::vector<T>::pointer>::type>
struct expose_genericvector
{
    typedef typename std::vector<T> vecT;
    //for at(), operator[]()
    typedef refType (vecT::*randaccess_fnptr)(typename vecT::size_type);
    
    //pop_front|back
    typedef void (vecT::*pop_fnptr)();

    static void Expose(lua_State* L, const char* name)
    {
        class_luadef<vecT>(L,name)
            .template add<randaccess_fnptr>("at",&vecT::at)
            .template add("reserve",&vecT::reserve)
            .template add("capacity",&vecT::capacity)
            .template add("shrink_to_fit",&vecT::shrink_to_fit)
            .template add<void (vecT::*)(typename vecT::size_type)>("resize",&vecT::resize);

        expose_concept_Container<vecT,itrtype>::Expose(L,name);
        expose_concept_SequenceContainer<vecT,itrtype,refType,ptrType>::Expose(L,name);
        expose_concept_ReversibleContainer<vecT,itrtype>::Expose(L,name);
        expose_generic_queue<vecT,refType>::Expose(L,name);

        std::string itrname = name;
        itrname.append("_iterator");
        expose_concept_RandomAccessIterator<itrtype>::Expose(L,itrname.c_str());
    }
};

template<typename T>
//Use this if you want it to use const_iterator when possible
struct expose_constvector
{
    typedef expose_genericvector<T,typename std::vector<T>::const_iterator> type;
};

template<typename T>
//Use this if you want it to use a non-const iterator when possible
struct expose_nonconstvector
{
    typedef expose_genericvector<T,typename std::vector<T>::iterator> type;
};


}
}
