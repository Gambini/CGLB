#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
//This is the file where comon stl related things exist
#include <type_traits>
#include <cglb/class_luadef.h>

namespace cglb {
namespace stl {

template<class T>
//since std::add_const doesn't work for reference types, this must be used.
//It removes the reference, adds the const and adds the reference back
struct add_constreference { 
    typedef typename std::add_lvalue_reference<
        typename std::add_const< typename std::remove_reference<T>::type >::type
    >::type type; 
};

}
}
