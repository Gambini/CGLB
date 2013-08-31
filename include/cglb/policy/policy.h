#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include <type_traits>
#include "return_gc.h"

namespace cglb {

template<typename ReturnGC = return_gc<std::false_type>>
struct policy
{
    static const bool ShouldGC = ReturnGC::should_gc;
};

}
