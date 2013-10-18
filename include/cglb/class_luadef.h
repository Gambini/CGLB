#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include "function_traits.h"
#include "function_def.h"
#include "class_luarep.h"
#include "memdat_def.h"
#include "lua_include.h"
#include "policy/return_gc.h"
#include "cglb_init.h"
#include <vector>
#include <string>
#include <type_traits>
#include <mutex>
#include <unordered_map>

namespace cglb {

template <typename T>
struct class_luadef
{
    /**
     * Constructor
     */
    class_luadef(lua_State* Ls, const char* class_name) :
        L(Ls),name(class_name)
    {
        std::lock_guard<std::mutex> lock(instantiate_mutex);
        if(class_luarep<T>::setup(L,class_name))
        {
            dealloc_functions.push_back(class_luadef<T>::DeallocateLuaDefs);
        }
    }


    
    static void DeallocateLuaDefs()
    {
        for(auto fnpair : registered_functions)
        {
            free(fnpair.second);
            registered_functions[fnpair.first] = nullptr;
        }
        class_luarep<T>::DeallocateDeleter();
        registered_functions.clear();
    }

    

    template<typename ParentClassT>
    class_luadef& inherit()
    {
        int top = lua_gettop(L);
        luaL_getmetatable(L,class_luarep<ParentClassT>::mt_name.c_str());
        int parent_mtbl = lua_gettop(L);
        luaL_getmetatable(L,class_luarep<T>::mt_name.c_str());
        int mtbl = lua_gettop(L);
        lua_pushnil(L);
        while(lua_next(L,parent_mtbl) != 0)
        {
            int keyindex = lua_gettop(L) - 1;
            if(lua_isstring(L,keyindex))
            {
                lua_setfield(L,mtbl,lua_tostring(L,keyindex));
            }
            else
            {
                lua_pop(L,1);
            }
        }
        lua_settop(L,top);
        return *this;
    }


    /**
     * Add a member function for <T>. <Func> must be a member function pointer, or just a regular
     * pointer that is _not_ to a member data. If it is a non-member function, the first
     * parameter of <Func> MUST be of type lua_State*. There will be no available overloads
     * of PushFunctionClosure if those conditions are not met.
     *
     * If the function is a non member function, then the possible function pointer types are
     * int (*)(lua_State*) and int (*)(lua_State*,T*), where the T* is grabbed from index 1
     * of the lua stack upon function call. See function_def.h, and the custom_function_def 
     * in that file.
     *
     * In Lua, it will be accessed with the colon (:) operator
     */
    template< typename Func, typename Traits = function_traits<Func> >
    typename std::enable_if<
                (std::is_pointer<Func>::value
             && !std::is_member_object_pointer<Func>::value) 
              || std::is_member_function_pointer<Func>::value,
    class_luadef& >::type
    add(const char* fname, Func f)
    {
        typedef policy<return_gc<std::false_type>> pol;

        //setup for closure
        auto mtn = class_luarep<T>::mt_name;
        lua_getglobal(L,mtn.c_str());               //[1] = metatable
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No metatable named %s. Global %s returned %s",mtn.c_str()
                , mtn.c_str(), luaL_typename(L,-1));
            return *this;
        }
        int mtidx = lua_gettop(L); 
        
        lua_pushstring(L,fname);

        PushFunctionClosure<Func,pol>(fname,f);
        lua_settable(L,mtidx);

        lua_pop(L,1); //pop metatable

        return *this;
    }

//helper functions for add(function)
private:

    /**
     * If the function passed in has a lua_State* as the first parameter, then we can assume
     * that the function wishes to manipulate the Lua stack itself rather than have the code
     * automatically generated.
     */
    template< typename Func, typename pol, typename Traits = function_traits<Func> >
    typename std::enable_if< !std::is_member_function_pointer<Func>::value
                        //The next check is required due to some functions having 0 arguments,
                        //and function_traits::arg<0> throwing a compilation error for them
                        &&   !std::is_same< 
                                typename std::integral_constant<size_t, 0            >::type,
                                typename std::integral_constant<size_t, Traits::arity>::type
                             >::value,
    void>::type
    PushFunctionClosure(const char* fn_name, Func f, 
        typename std::enable_if< 
            std::is_same< typename Traits::template arg<0>::type, lua_State* >::value 
        >::type* = 0)
    {
        typedef custom_function_def<T,pol,Func> CustomFnT;
        CustomFnT* fndef = nullptr;

        auto itr = registered_functions.find(fn_name);
        if(itr != registered_functions.end())
        {
            fndef = (CustomFnT*)(itr->second);
        }
        else
        {
            fndef = (CustomFnT*)malloc(sizeof(CustomFnT));
            fndef->SetFnPtr(f);
            registered_functions.insert(std::pair<std::string,void*>(fn_name,(void*)fndef));
        }
        
        lua_pushlightuserdata(L,(void*)fndef);
        lua_pushcclosure(L,&CustomFnT::LuaFunction,1);
    }


    /**
     * Similar to the overload above, but for standalone non-member functions
     */
    template< typename Func, typename pol, typename Traits = function_traits<Func> >
    typename std::enable_if< !std::is_member_function_pointer<Func>::value
                        //The next check is required due to some functions having 0 arguments,
                        //and function_traits::arg<0> throwing a compilation error for them
                        &&   !std::is_same< 
                                typename std::integral_constant<size_t, 0            >::type,
                                typename std::integral_constant<size_t, Traits::arity>::type
                             >::value,
    void>::type
    PushFunctionClosure(const char* fn_name, Func f, 
        typename std::enable_if< 
            !std::is_same< typename Traits::template arg<0>::type, lua_State* >::value 
        >::type* = 0)
    {
        typedef function_def<Traits,Func,pol> FnDefT;
        FnDefT* fndef = nullptr;

        auto itr = registered_functions.find(fn_name);
        if(itr != registered_functions.end())
        {
            fndef = (FnDefT*)(itr->second);
        }
        else
        {
            fndef = (FnDefT*)malloc(sizeof(FnDefT));
            fndef->fnptr = f;
            registered_functions.insert(std::pair<std::string,void*>(fn_name,(void*)fndef));
        }
        
        lua_pushlightuserdata(L,(void*)fndef);
        lua_pushcclosure(L,&FnDefT::LuaFunction,1);
    }
   

    /**
     * This is a catch-all for member functions, where the code that interoperates with
     * the Lua stack is generated by this library.
     */
    template< typename Func, typename pol, typename Traits = function_traits<Func> >
    typename std::enable_if< std::is_member_function_pointer<Func>::value,
    void >::type
    PushFunctionClosure(const char* fn_name, Func f, typename std::enable_if<true>::type* = 0)
    {
        typedef function_def<Traits,Func,pol> FnDefT;
        FnDefT* fndef = nullptr;

        auto itr = registered_functions.find(fn_name);
        if(itr != registered_functions.end())
        {
            fndef = (FnDefT*)(itr->second);
        }
        else
        {
            fndef = (FnDefT*)malloc(sizeof(FnDefT));
            fndef->fnptr = f;
            registered_functions.insert(std::pair<std::string,void*>(fn_name,(void*)fndef));
        }

        lua_pushlightuserdata(L,(void*)fndef);
        lua_pushcclosure(L,&FnDefT::LuaFunction,1);
    }



    /**
     * Helper for the member data adding functions, to reduce code duplication
     *
     * Will either retrieve a previously allocated object, or allocate a new one
     */
    template< typename MemDatPtr, typename Traits = memdat_traits<MemDatPtr> >
    memdat_def<MemDatPtr,Traits>* GetMemDatFunction(const char* dname, MemDatPtr memdat)
    {
        typedef memdat_def<MemDatPtr,Traits> MemDatT;

        auto itr = registered_functions.find(dname);
        if(itr != registered_functions.end())
        {
            return (MemDatT*)(itr->second);
        }
        else
        {
            MemDatT* mdef = (MemDatT*)malloc(sizeof(MemDatT));
            mdef->memptr = memdat;
            registered_functions.insert(std::pair<std::string,void*>(dname,(void*)mdef));
            return mdef;
        }
    }
    
public:

    /**
     * Add a member data. <MemDatPtr> must be a pointer to member data.
     * The same as adding read and write.
     *
     * In Lua, it will be accessed by the dot (.) operator
     */
    template< typename MemDatPtr >
    typename std::enable_if< std::is_member_object_pointer<MemDatPtr>::value,
    class_luadef& >::type
    add(const char* dname, MemDatPtr memdat)
    {
        typedef memdat_traits<MemDatPtr> Traits;
        typedef memdat_def<MemDatPtr,Traits> MemDatT;

        lua_getglobal(L,class_luarep<T>::mt_name.c_str());
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No metatable named %s",class_luarep<T>::mt_name.c_str());
            return *this;
        }
        int mtidx = lua_gettop(L);

        lua_pushstring(L,"__cglb_getters");
        lua_rawget(L,mtidx);
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No __cglb_getters for %s", class_luarep<T>::class_name.c_str());
            return *this;
        }
        int getteridx = lua_gettop(L);

        
        auto* md = GetMemDatFunction(dname,memdat);
        lua_pushlightuserdata(L,(void*)md);

        lua_pushvalue(L,-1);
        int uval = lua_gettop(L);

        lua_pushstring(L,dname);
        lua_pushvalue(L,uval);
        lua_pushcclosure(L,&MemDatT::LuaGetFunction,1);
        lua_settable(L,getteridx);


        lua_pushstring(L,"__cglb_setters");
        lua_rawget(L,mtidx);
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No __cglb_setters for %s", class_luarep<T>::class_name.c_str());
            return *this;
        }
        int setteridx = lua_gettop(L);

        lua_pushstring(L,dname);
        lua_pushvalue(L,uval);
        lua_pushcclosure(L,&MemDatT::LuaSetFunction,1);
        lua_settable(L,setteridx);


        lua_settop(L,mtidx-1); //clean stack
        return *this;
        
    }

    
    /**
     * Adds a get (__index) method, but not a set method.
     *
     * Accessed in Lua by the dot (.) operator.
     */
    template< typename MemDatPtr >
    typename std::enable_if< std::is_member_object_pointer<MemDatPtr>::value,
    class_luadef& >::type
    add_readonly(const char* dname, MemDatPtr memdat)
    {
        typedef memdat_traits<MemDatPtr> Traits;
        typedef memdat_def<MemDatPtr,Traits> MemDatT;

        lua_getglobal(L,class_luarep<T>::mt_name.c_str());
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No metatable named %s",class_luarep<T>::mt_name.c_str());
            return *this;
        }
        int mtidx = lua_gettop(L);

        lua_pushstring(L,"__cglb_getters");
        lua_rawget(L,mtidx);
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No __cglb_getters for %s", class_luarep<T>::class_name.c_str());
            return *this;
        }
        int getteridx = lua_gettop(L);

        
        auto* md = GetMemDatFunction(dname,memdat);
        lua_pushlightuserdata(L,(void*)md);

        lua_pushvalue(L,-1);
        int uval = lua_gettop(L);

        lua_pushstring(L,dname);
        lua_pushvalue(L,uval);
        lua_pushcclosure(L,&MemDatT::LuaGetFunction,1);
        lua_settable(L,getteridx);

        lua_settop(L,mtidx-1);
        return *this;
    }


    /**
     * Adds a set method, but not a get method.
     *
     * Accessed in Lua by the dot (.) operator
     */
    template< typename MemDatPtr >
    typename std::enable_if< std::is_member_object_pointer<MemDatPtr>::value,
    class_luadef& >::type
    add_writeonly(const char* dname, MemDatPtr memdat)
    {
        typedef memdat_traits<MemDatPtr> Traits;
        typedef memdat_def<MemDatPtr,Traits> MemDatT;

        lua_getglobal(L,class_luarep<T>::mt_name.c_str());
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No metatable named %s",class_luarep<T>::mt_name.c_str());
            return *this;
        }
        int mtidx = lua_gettop(L);

        lua_pushstring(L,"__cglb_setters");
        lua_rawget(L,mtidx);
        if(lua_isnoneornil(L,-1))
        {
            luaL_error(L,"No __cglb_setters for %s", class_luarep<T>::class_name.c_str());
            return *this;
        }
        int setteridx = lua_gettop(L);

        
        auto* md = GetMemDatFunction(dname,memdat);
        lua_pushlightuserdata(L,(void*)md);

        lua_pushvalue(L,-1);
        int uval = lua_gettop(L);

        lua_pushstring(L,dname);
        lua_pushvalue(L,uval);
        lua_pushcclosure(L,&MemDatT::LuaSetFunction,1);
        lua_settable(L,setteridx);

        lua_settop(L,mtidx-1);
        return *this;
    }


    
   /**
    * This should only need to be defined if would like to have it do something
    * other than just call "delete" upon garbage collection
    * 
    * <FnPtr> Cannot be a member function pointer.
    */
    template<typename FnPtr>
    typename std::enable_if<std::is_pointer<FnPtr>::value
                           && std::is_function<typename std::remove_pointer<FnPtr>::type>::value
                           && !std::is_member_function_pointer<FnPtr>::value,
    class_luadef&>::type
    destructor(FnPtr fnptr)
    {
        class_luarep<T>::set_deleter(L,fnptr);
        return *this;
    }


    /**
     * A template overload to give nice errors about usage.
     */
    template<typename FnPtr>
    typename std::enable_if<
                          std::is_pointer<FnPtr>::value
                       && std::is_function<typename std::remove_pointer<FnPtr>::type>::value
                       && std::is_member_function_pointer<FnPtr>::value,
    class_luadef&>::type
    destructor(FnPtr fnptr)
    {
        static_assert(std::is_member_function_pointer<FnPtr>::value,
            "Custom destructor for lua type cannot be a member function pointer.");
        return *this;
    }

    /**
     * The template parameters are the used to define the argument types passed to the 
     * constructor of <T>
     */
    template<typename... Args>
    class_luadef& constructor()
    {
        //need the metatable since we are setting a metafunction
        luaL_newmetatable(L,class_luarep<T>::mt_name.c_str());
        int mtidx = lua_gettop(L);
        
        typedef function_traits<
            decltype(&class_luarep<T>::template constructor<Args...>::ConstructType)
            > Traits;
        typedef decltype(&class_luarep<T>::template constructor<Args...>::ConstructType) FuncT;
        typedef policy<return_gc<std::true_type>> pol; //Constructor should always delete on GC
        typedef function_def<Traits,FuncT,pol> FnDefT;
        
        auto* fd = (FnDefT*)lua_newuserdata(L,sizeof(FnDefT));
        fd->fnptr = &class_luarep<T>::template constructor<Args...>::ConstructType;


        lua_pushcclosure(L,&FnDefT::LuaFunction,1);
        lua_setfield(L,mtidx,"__init");
        
        lua_getfield(L,mtidx,"__init");
        //set it so that you can use the name of the class
        //as a C++-like constructor in Lua
        lua_setglobal(L,name); 

        lua_settop(L,mtidx-1); //stack cleanup
        return *this;
    }


    /**
     * Cannot be a member function
     */
    template<typename Func>
    typename std::enable_if<!std::is_member_function_pointer<Func>::value,
    class_luadef&>::type
    custom_constructor(Func f)
    {
        typedef policy<return_gc<std::true_type>> pol;
        luaL_newmetatable(L,class_luarep<T>::mt_name.c_str());
        int mtidx = lua_gettop(L);
        PushFunctionClosure<Func,pol>("__init",f);
        lua_setfield(L,mtidx,"__init"); // as a metamethod
        PushFunctionClosure<Func,pol>(name,f);
        lua_setglobal(L,name); //and as a more C-like constructor syntax
        
        lua_settop(L,mtidx-1);
        return *this;
    }


//metamethods
public:
    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //Lua will pass in both arguments in order, so if you have "1 + class" and have a member
    //function defined as opAdd, then it will pick up "1" as the class type
    class_luadef& opAdd(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__add",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //Lua will pass in both arguments in order, so if you have "1 - class" and have a member
    //function defined as opSub, then it will pick up "1" as the class type
    class_luadef& opSub(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__sub",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //unary minus
    class_luadef& opUnm(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__unm",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //multiply
    class_luadef& opMul(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__mul",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //divide
    class_luadef& opDiv(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__div",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //modulus
    class_luadef& opMod(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__mod",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //power
    class_luadef& opPow(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__pow",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //concat
    class_luadef& opConcat(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__concat",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //equality
    class_luadef& opEq(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__eq",fnptr);
    }


    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //less than
    class_luadef& opLt(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__lt",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //less than or equal
    class_luadef& opLe(FnPtr fnptr)
    {
        typedef policy<return_gc<std::true_type>> pol;
        return opMeta<pol>("__le",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //operator ()
    class_luadef& opCall(FnPtr fnptr)
    {
        return opMeta<policy_return_gc>("__call",fnptr);
    }

    template<typename FnPtr, typename Traits = function_traits<FnPtr>>
    //operator # 
    class_luadef& opLen(FnPtr fnptr)
    {
        return opMeta<policy_return_gc>("__len",fnptr);
    }


    lua_State* L;
    const char* name; //Name of the class. Used as the name of the constructor
    //Needed to make class_luarep<T>::Setup thread safe
    static std::mutex instantiate_mutex;


public:
    /**
     * Helper function to all of the meta functions. Contains all of the
     * common boilerplate things for interacting with Lua metamethods
     */
    template<typename pol, typename FnPtr, typename Traits = function_traits<FnPtr>>
    class_luadef& opMeta(const char* metaname, FnPtr fnptr)
    {
        luaL_newmetatable(L,class_luarep<T>::mt_name.c_str());
        int mtidx = lua_gettop(L);

        PushFunctionClosure<FnPtr,pol>(metaname, fnptr);
        lua_setfield(L,mtidx,metaname);

        lua_settop(L,mtidx-1);

        return *this;
    }






private:
    /**
     * Usually a void* is a bad idea, but due to LuaJIT on x64 linux having a 2GB memory
     * limit, lightuserdata is used rather than full userdata. This also has the benefit
     * of sharing Lua definitions across Lua states rather than allocating new function 
     * objects for each state.
     *
     * The key is the function name, and the value is the function_def or
     * custom_function_def pointer allocated by malloc in one of the .add functions.
     *
     * The type does not matter, this is only used to check if it has been registered before, 
     * and to delete it upon program exit.
     */
    static std::unordered_map<std::string,void*> registered_functions;
};

template<typename T>
std::unordered_map<std::string,void*> class_luadef<T>::registered_functions;

template<typename T>
std::mutex class_luadef<T>::instantiate_mutex;

}
