#pragma once
/*
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include <vector>
#include <functional>
namespace cglb {

static bool record_types = false;
static std::vector<std::function<void(void)>> dealloc_functions;

/**
 * If you wish to have a quick and easy way to deallocate all of the
 * memory in one call, then this function must be called before any of the 
 * `class_luadef` functions are called. See the documentation for "Quit" for more info.
 */
inline void Init()
{
    record_types = true;
}


/**
 * `Init` *MUST* be called for this to work.
 *
 * If this function is not used, then you will have to call
 * `class_luadef<T>::DeallocateLuaDefs()` for every type which is exposed if you wish to
 * free all memory alloced by this library.
 */
inline void Quit()
{
    for(auto& fn : dealloc_functions)
    {
        fn();
    }
    dealloc_functions.clear();
    record_types = false;
}

}
