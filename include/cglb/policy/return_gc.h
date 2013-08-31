#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include <type_traits>

namespace cglb {


template<typename T>
struct return_gc 
{
};




template<>
struct return_gc<std::true_type> 
{
    static const bool should_gc = true;
};

template<>
struct return_gc<std::false_type>
{
    static const bool should_gc = false;
};

}
