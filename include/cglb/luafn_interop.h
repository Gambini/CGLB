#pragma once
#include "function_traits.h"
#include "class_luarep.h"
#include "lua_include.h"
#include <type_traits>

namespace cglb {
namespace detail {


    template<typename T
        , typename strippedT = typename std::decay< 
            typename std::remove_reference<T>::type 
        >::type>
    //since lua handles all numbers the same way, we just need to check if it
    //is an integral or floating point type. This also catches 'bool'
    static typename std::enable_if<
        std::is_arithmetic<strippedT>::value
     || std::is_enum<strippedT>::value,
    strippedT >::type
    GetFuncArg(lua_State* L, int idx)
    {
        return static_cast<strippedT>(luaL_checknumber(L,idx));
    }


    /**
     * This is going to get a little bit wonky. A restriction placed on this library
     * is that any user-defined type must exist in Lua as a pointer. You cannot push
     * value types to Lua. However, functions can take non-pointer types, so the following
     * few overloads are filtering which conversions need to happen
     * 
     * Something to remember: template T is not the argument in Lua, but the argument
     * type for the C++ function
     */
    template<typename Tptr
            , typename T = typename std::decay<typename std::remove_pointer<Tptr>::type>::type>
    //Handle podt pointers (not sure if any other types match this)
    static typename std::enable_if< 
        std::is_pointer<Tptr>::value  
        && !std::is_same<const char*,Tptr>::value 
        && !std::is_class<T>::value,
    Tptr >::type
    GetFuncArg(lua_State* L, int idx)
    {
        return static_cast<Tptr>(lua_topointer(L,idx));
    }


    //Handle pointers to classes ie. func(T* arg) or func(const T* arg)
    template<typename Tptr
            , typename T = typename std::decay< typename std::remove_pointer<Tptr>::type >::type>
    static typename std::enable_if<std::is_pointer<Tptr>::value && std::is_class<T>::value,
    Tptr >::type
    GetFuncArg(lua_State* L, int idx)
    {
        return class_luarep<T>::check(L,idx);
    }


    template<typename Tref
            , typename T = typename std::decay< typename std::remove_reference<Tref>::type >::type>
    //Handle references to class ie. func(T& arg) or func(const T& arg)
    static typename std::enable_if<
            std::is_reference<Tref>::value  
         && std::is_class<T>::value
         && !std::is_same<T,std::string>::value, //strings are special types for Lua
    Tref >::type
    GetFuncArg(lua_State* L, int idx)
    {
        T* arg = class_luarep<T>::check(L,idx);
        return *arg; //is this right? Or can you directly pass a pointer as a reference
    }


    template<typename T, typename DecayT = typename std::decay<T>::type>
    //Handle non-ref function args of a class type
    static typename std::enable_if<!std::is_reference<T>::value  
                              && std::is_class<DecayT>::value
                              && !std::is_same<DecayT,std::string>::value,
    T >::type
    GetFuncArg(lua_State* L, int idx)
    {
        T* arg = class_luarep<T>::check(L,idx);
        return *arg; 
    }


    template<typename T>
    //handle strings
    static typename std::enable_if<std::is_same<const char*,T>::value
                      || std::is_same<std::string,typename std::decay<T>::type>::value,
    const char* >::type
    GetFuncArg(lua_State* L, int idx)
    {
        return luaL_checkstring(L,idx);
    }


    


    template<typename T, typename pol
        , typename RmPtrT = typename std::decay< typename std::remove_pointer<T>::type >::type>
    static typename std::enable_if<
        (std::is_arithmetic<RmPtrT>::value 
        && !std::is_same<bool,T>::value
        && !std::is_same<char,RmPtrT>::value)
        || std::is_enum<T>::value
    >::type
    PushFuncResult(lua_State* L, T res)
    {
        lua_pushnumber(L,lua_Number(res));
    }


    template<typename T, typename pol>
    static typename std::enable_if<std::is_same<bool,T>::value>::type
    PushFuncResult(lua_State* L, T res)
    {
        lua_pushboolean(L,res ? 1 : 0);
    }


    template<typename Tptr, typename pol
            , typename T = typename std::decay<typename std::remove_pointer<Tptr>::type>::type>
    static typename std::enable_if<std::is_pointer<Tptr>::value 
                                  && std::is_class<T>::value>::type
    PushFuncResult(lua_State* L, Tptr res)
    {
        class_luarep<T>::push(L,res,pol::ShouldGC); //doesn't handle garbage-collection from constructors
    }


    template<typename Tref, typename pol, typename T = typename std::decay<Tref>::type>
    static typename std::enable_if<std::is_reference<Tref>::value 
                              && std::is_class<T>::value
                              && !std::is_same<T,std::string>::value
    >::type
    PushFuncResult(lua_State* L, Tref res)
    {
        class_luarep<T>::push(L,res,pol::ShouldGC); //references can be pushed as pointers
    }


    //Since it is temporary (pass by value) anyway, the user shouldn't care so much what kind 
    //of type is passed to lua. However, it is not possible to pass non-pointer types to Lua in 
    //this library, so make sure that it is copy constructorable
    template<typename T, typename pol, typename DecayT = typename std::decay<T>::type>
    //this is a value result with a class type (i.e. non-pod)
    static typename std::enable_if<
            !std::is_reference<T>::value 
         && !std::is_pointer<T>::value
         &&  std::is_copy_constructible<DecayT>::value
         &&  std::is_class<DecayT>::value
         && !std::is_same<DecayT,std::string>::value
    >::type
    PushFuncResult(lua_State* L, T res)
    {
        //FIXME: Use the allocation strategy of the user, because of the ability to
        //use malloc/free rather than new/delete in constructor/destructor of lua_classdef<T>
        T* resPtr = new T(res);
        class_luarep<T>::push(L,resPtr,true); //have it be garbage collected
    }


    template<typename T, typename pol, typename DecayT = typename std::decay<T>::type>
    //this is a value result with a class type (i.e. non-pod), which is not copy constructable
    //This probably can't happen, but better safe than sorry
    static typename std::enable_if<!std::is_reference<T>::value 
                                  && !std::is_pointer<T>::value
                                  && !std::is_copy_constructible<DecayT>::value
                                  && std::is_class<DecayT>::value
    >::type
    PushFuncResult(lua_State* L, T res)
    {
        static_assert(!std::is_copy_constructible<DecayT>::value,
            "Could not push the result of a function on to the Lua stack due to the type not being "
            "copy constructible. Change it to be a reference or pointer type.");
    }



    template<typename T, typename pol>
    static typename std::enable_if<std::is_same<const char*,T>::value>::type
    PushFuncResult(lua_State* L, T res)
    {
        lua_pushstring(L,res);
    }

    template<typename T, typename pol>
    static typename std::enable_if<
        std::is_same<typename std::decay<T>::type,std::string>::value
    >::type
    PushFuncResult(lua_State* L, T res)
    {
        return PushFuncResult<const char*,pol>(L,res.c_str());
    }






    //Non-value returning member function
    template<typename T, typename FnPtrT, typename Traits, typename pol
             , typename... Args, typename R = typename Traits::result_type>
    static typename std::enable_if< std::is_same<R,void>::value,int >::type
    /*static int*/ MemberFunctionCall(lua_State* L, T* self, FnPtrT fnptr, Args ... a)
    {
        (self->*fnptr)(a ...);
        return 0;
    }

    //Value-returning member function
    template<typename T, typename FnPtrT, typename Traits, typename pol
            , typename... Args, typename R = typename Traits::result_type>
    static typename std::enable_if< !std::is_same<R,void>::value,int >::type
    /*static int*/ MemberFunctionCall(lua_State* L, T* self, FnPtrT fnptr, Args ... a)
    {
        R ret = (self->*fnptr)(a ...);
        PushFuncResult<R,pol>(L,ret);
        return 1;
    }



    //non-value returning function
    template<typename FnPtrT, typename Traits, typename pol, typename... Args
            , typename R = typename Traits::result_type>
    static typename std::enable_if< std::is_same<R,void>::value,int >::type
    /*static int*/ FunctionCall(lua_State* L, FnPtrT fnptr, Args ... a)
    {
        (*fnptr)(a ...);
        return 0;
    }

    //value returning function
    template<typename FnPtrT, typename Traits, typename pol, typename... Args
            , typename R = typename Traits::result_type>
    static typename std::enable_if< !std::is_same<R,void>::value,int >::type
    /*static int*/ FunctionCall(lua_State* L, FnPtrT fnptr, Args ... a)
    {
        R ret = (*fnptr)(a ...);
        PushFuncResult<R,pol>(L,ret);
        return 1;
    }






    template<int N>
    struct GatherArgs
    {
        template<typename FnPtrT, typename Traits, typename pol, typename... Args
                , typename O = 
                    typename std::conditional<
                    std::is_member_function_pointer<FnPtrT>::value,
                    std::integral_constant<int,0>::type,
                    std::integral_constant<int,1>::type>
                ::type>
        static int Gather(lua_State* L, FnPtrT fptr, Args ... a)
        {
            typedef typename Traits::template arg<(size_t)(N-2)>::type FnArgT;
            //If it is a member function pointer, then there is one more object on the stack
            //than there are arguments for the function call to fptr.
            FnArgT arg = GetFuncArg<FnArgT>(L,N - O::value);
            return GatherArgs<N-1>::template Gather<FnPtrT,Traits,pol>(L,fptr,arg,a ...);
        }
    };


    template <>
    struct GatherArgs<1>
    {
        template <typename FnPtrT, typename Traits, typename pol, typename... Args>
        static typename std::enable_if<std::is_member_function_pointer<FnPtrT>::value,int>::type
        /*static int*/ Gather(lua_State* L, FnPtrT fptr, Args ... a)
        {
            typedef typename std::decay<typename Traits::owner_type>::type T;
            //type instance is always the first argument
            T** holdPtr = static_cast<T**>(lua_touserdata(L,1));
            if(holdPtr == NULL)
            {
                lua_pushnil(L);
                return 1;
            }
            T* self = *holdPtr;
            return MemberFunctionCall<T,FnPtrT,Traits,pol>(L,self,fptr,a...);    
        }

        template <typename FnPtrT, typename Traits, typename pol, typename... Args>
        static typename std::enable_if<!std::is_member_function_pointer<FnPtrT>::value,int>::type
        /*static int*/ Gather(lua_State* L, FnPtrT fptr, Args ... a)
        {
            return FunctionCall<FnPtrT,Traits,pol>(L,fptr,a...);
        }
        
    };


}
}
