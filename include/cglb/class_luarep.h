#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include "lua_include.h"
#include <vector>
#include <assert.h>
#include <algorithm>

namespace cglb {

template<typename T>
struct class_luadef;

template<typename T>
void default_classrep_deleter(T* obj);


/**
 * This is the interface between C++ and Lua.
 */
template <typename T>
struct class_luarep
{
    //invalid default names for Lua, so this shouldn't clash 
    static std::string class_name;
    static std::string mt_name;


    /**
     * Pushes obj on to the stack with the Lua definition given from
     * class_luadef<T>'s "add" methods.
     * Set gc to true if the object should be destroyed upon garbage collection
     *
     * Returns the index of the Lua stack where obj resides
     */
    static int push(lua_State* L, T* obj, bool gc = false)
    {
        if(!obj) //can't very well push a null value
        {
            lua_pushnil(L);
            return lua_gettop(L);
        }

        luaL_getmetatable(L, mt_name.c_str());                   //[1] = metatable
        if(lua_isnoneornil(L,-1))
        {
            //If this error says that it is missing the metatable for '@', then
            //the type <T> has not been defined by class_luadef<T>
            luaL_error(L,"%s missing metatable", class_name.c_str());
        }

        int mtidx = lua_gettop(L);                              //[mtidx] = [1]
        T** ptrHold = (T**)lua_newuserdata(L,sizeof(T**));      //[2] = userdata
        int udidx = lua_gettop(L);                              //[udidx] = [2]
        if(ptrHold != NULL)
        {
            *ptrHold = obj;
            lua_pushvalue(L,mtidx);                             //[3] = [1]
            lua_setmetatable(L,udidx);                          //setmetatable([2],[3])     -> pop[3]

            //set the garbage collection data
            char objname[32];
            sprintf(objname,"%p",obj);
            luaL_newmetatable(L,"DO NOT TRASH");                //[3] = "DO NOT TRASH" table
            if(!gc)
            {
                lua_pushboolean(L,1);                           //[4] = true
                lua_setfield(L,-2,objname);                     //[3][name] = [4]           -> pop[4]
            }
            else
            {
                lua_pushnil(L);                                 //[4] = nil
                lua_setfield(L,-2,objname);                     //[3][name] = [4]           -> pop[4]
            }
            lua_pop(L,1);                                       //pop[3]
        }
        lua_settop(L,udidx);                                    //pop[>2]
        lua_replace(L,mtidx);                                   //swap([mtidx],[udidx])     -> pop[mtidx]
        udidx = mtidx;
        lua_settop(L,udidx);                                    //pop[>1], just to make sure
        return udidx;
    }



    /**
     * For getting user type data from the Lua stack
     */
    static T* check(lua_State* L, int narg)
    {
        T** ptrHold = static_cast<T**>(lua_touserdata(L,narg));
        if(ptrHold == NULL)
            return NULL;
        return *ptrHold;
    }


    /**
     * Used as the __init metamethod. Instanced from
     * class_luadef<T>::constructor<...>()
     *
     * Note that this uses "new" for allocation. If malloc is the preferred allocator, then
     * class_luadef<T>::custom_constructor(function) is what you want.
     */
    template<typename... Args>
    struct constructor
    {
        //creates/allocates a new T
        static T* ConstructType(Args... a)
        {
            T* ret = new T(a...);
            return ret;
        }
    };



private:
    friend struct class_luadef<T>;


    /**
      * Called from class_luadef<T> constructor.
      */
    static void setup(lua_State* L, const char* name)
    {
        class_name = name;
        mt_name = name;
        mt_name.append("_mt");
        Register(L);
    }

    /**
     * The restrictions for this class is that the function pointer
     * cannot be a member function, and it must take a single argument
     * which is a pointer to the data of type <T> to destroy
     *
     * The default deleter (default_classrep_deleter<T>) just calls delete on the object, so
     * if you use malloc/free rather than new/delete, then you will need to use the
     * class_luadef<T>::destructor(function) call
     *
     * If class_luarep<T>::push function was called and the "gc" argument was false,
     * then the deleter will not be called.
     */
    template<typename FnPtr>
    struct deleter
    {
        deleter(FnPtr fp) : delete_func(fp)
        {}
        
        FnPtr delete_func;
        /*
            Upvalue index 1 is an instance of this class
        */
        static int gc_metamethod(lua_State* L)
        {
            T* obj = check(L,1);                                //[1] = T* instance
            if(obj == NULL)
                return 0;

            luaL_newmetatable(L,"DO NOT TRASH");                //[2] = "DO NOT TRASH" table
            char objname[32];
            sprintf(objname,"%p",obj);
            lua_getfield(L,-1,objname);                         //[3] = [2][objname]
            if(lua_isnoneornil(L,-1))
            {
                deleter<FnPtr>* self = (deleter<FnPtr>*)current_deleter;
                (*(self->delete_func))(obj);
                lua_pushboolean(L,1);                           //[4] = true
                lua_setfield(L,-3,objname);                     //[2][objname] = [4]        -> pop[4]
            }
            lua_pop(L,3);                                       //pop[3,2,1]
            return 0;
        }
    };


    template<typename Func>
    static void set_deleter(lua_State* L, Func f)
    {
        if(current_deleter != nullptr)
        {
            free(current_deleter);
            current_deleter = nullptr;
        }
        
        luaL_newmetatable(L,mt_name.c_str());                   //[1] = mt
        int metaidx = lua_gettop(L);
        deleter<Func>* fn = (deleter<Func>*)malloc(sizeof(deleter<Func>));
        current_deleter = (void*)fn;
        if(fn != NULL)
        {
            fn->delete_func = f;
            lua_pushcfunction(L,deleter<Func>::gc_metamethod);
            lua_setfield(L,metaidx,"__gc");                     //pop[2]
        }
        lua_settop(L,metaidx-1); //stack cleanup
    }

    /**
     * Defines the pointer to a deleter<?> object, hence the void*
     */
    static void* current_deleter;

    /**
     * For use after the library is shut down. If this is not called,
     * then it is a memory leak.
     */
    static void DeallocateDeleter()
    {
        free(current_deleter);
        current_deleter = nullptr;
    }



    /**
     * Checks to see if Register has been called for this lua_State before, and if not,
     * registers the basic metamethods.
     */
    static void Register(lua_State* L)
    {
        lua_getglobal(L,mt_name.c_str());
        //See if it has been registered already
        if(!lua_isnoneornil(L,-1))
        {
            lua_pop(L,1);
            return;
        }
        lua_pop(L,1);
        lua_newtable(L);                                //[1] = table
        int methods = lua_gettop(L);                    //methods = [1]

        //We use mt_name here so that we can have a 
        //C-looking constructor function that is the
        //name of the type
        luaL_newmetatable(L,mt_name.c_str());           //[2] = table in the registry named $mt_name
        int metaidx = lua_gettop(L);                    //metaidx = [2]

        luaL_newmetatable(L,"DO NOT TRASH");            //[3] = table in the registry named "DO NOT TRASH"
        lua_pop(L,1);                                   //pop[3]

        lua_pushvalue(L,methods);                       //[3] = methods
        lua_setglobal(L,mt_name.c_str());               //["_G"][mt_name] = [3]                 -> pop[3]

        //make it so that [get|set]metatable returns
        //the methods table
        lua_pushvalue(L,methods);                       //[3] = methods
        lua_setfield(L,metaidx,"__metatable");          //[metaidx]["__metatable"] = [methods]  -> pop[3]

        set_deleter(L,&default_classrep_deleter<T>);    //stack unchanged, sets __gc metamethod

        lua_pushcfunction(L,tostring);                  //[3] = this::tostring
        lua_setfield(L,metaidx,"__tostring");           //pop[3]

        lua_pushcfunction(L,index);                     //[3] = this::index
        lua_setfield(L,metaidx,"__index");              //pop[3]

        lua_pushcfunction(L,newindex);                  //[3] = this::newindex
        lua_setfield(L,metaidx,"__newindex");           //pop[3]

        lua_newtable(L);
        lua_setfield(L,methods,"__cglb_getters");
        lua_newtable(L);
        lua_setfield(L,methods,"__cglb_setters");

        //Why can this be empty?
        lua_newtable(L);                                //[3] = table
        lua_setmetatable(L,methods);                    //setmetatable(["_G"][mt_name],[3])     -> pop[3]
       
        lua_settop(L,methods - 1);                      //pop[>=methods]
    }

public:
    //__tostring
    static int tostring(lua_State* L)
    {
        char buff[32];
        T** ptrHold = (T**)lua_touserdata(L,1);
        T* obj = *ptrHold;
        sprintf(buff,"%p",obj);
        lua_pushfstring(L, "%s (%s)", class_name.c_str(), buff);//[1] = string
        return 1;
    }


    static int index(lua_State* L)
    {
        /* Upon calling this function, Lua has placed on the stack:
                                                              [1] = table/userdata
                                                              [2] = key
        */
        int objidx = lua_gettop(L) - 1;
        int keyidx = lua_gettop(L);
        lua_getglobal(L,mt_name.c_str());                   //[3] = _G[mt_name] table
        std::string strkey = lua_tostring(L,keyidx);
        if(lua_istable(L,-1))
        {
            lua_pushvalue(L,keyidx);                        //[4] = key
            lua_rawget(L,-2);                               //[4] = [mt_name][key]
            //if they key is not in the immediate table
            if(lua_isnoneornil(L,-1))
            {
                lua_pop(L,1);                               //pop[4] (nil)
                lua_pushstring(L,"__cglb_getters");         //[4] = string
                lua_rawget(L,-2);                           //[4] = custom __getters metamethod
                //we are now working on the __cglb_getters table
                lua_pushvalue(L,keyidx);                    //[5] = key
                lua_rawget(L,-2);                           //[5] = __cglb_getters[key]
                if(lua_type(L,-1) == LUA_TFUNCTION)
                {
                    int before_fcalltop = lua_gettop(L) - 1;//-1 to go below the function
                    lua_pushvalue(L,objidx);                //[6] = table/userdata
                    lua_pushvalue(L,keyidx);                //[7] = key
                    if(lua_pcall(L,2,LUA_MULTRET,0) != 0)   //[6+] = result of the function
                    {
                        luaL_error(L,"%s.__index for %s",class_name.c_str(),lua_tostring(L,2));
                    }
                    else
                    {
                        return lua_gettop(L) - before_fcalltop;
                    }
                }
                //it isn't a getter, check __index
                else
                {
                    lua_settop(L,4);                        //pop[>4]
                    lua_getmetatable(L,objidx);             //[5] = metatable for [1]
                    //if there is a metatable
                    if(lua_istable(L,-1))
                    {
                        lua_pushstring(L,"__index");        //[6] = "__index"
                        lua_rawget(L,-2);                   //[6] = mt.__index
                        if(lua_isfunction(L,-1))
                        {
                            int before_fcalltop = lua_gettop(L) - 1; //-1 to go below the function
                            lua_pushvalue(L,-2);            //[7] = mt (rather than userdata)
                            lua_pushvalue(L,keyidx);        //[8] = key
                            if(lua_pcall(L,2,1,0) != 0)     //[7] = result of the function
                            {
                                luaL_error(L,"%s.__index for %s",class_name.c_str(),lua_tostring(L,2));
                            }
                            else
                            {
                                return lua_gettop(L) - before_fcalltop;
                            }

                        }
                        else if(lua_istable(L,-1))
                        {
                            lua_pushvalue(L,keyidx);        //[7] = key
                            lua_rawget(L,-2);               //[7] = __index[key]
                        }
                        else
                        {
                            lua_pushnil(L);                 //[7] = nil
                        }
                    }
                    //if it doesn't have a metatable, then
                    //the key does not exist for the object
                    else 
                    {
                        lua_pushnil(L);                     //[6] = nil
                    }
                }
            }
            //if we can get it directly from the table
            else if(lua_istable(L,-1)) 
            {
                lua_pushvalue(L,keyidx);                    //[5] = key
                lua_rawget(L,-2);                           //[5] = [mt][key]
            } 
            //if it is anything other than nil or table, 
            //then the value is already at the top of the stack
        }
        else //if _G[mt_name] != table
        {
            lua_pushnil(L);                                 //[4] = nil
        }

        return 1;
    }



    static int newindex(lua_State* L)
    {
        /*  When this function is called, the top three items on the stack are
                                                              [1] = table/userdata
                                                              [2] = key
                                                              [3] = value
        */
        //check  to see if the key is in our custom __setters metafunction
        int validx = lua_gettop(L);
        int keyidx = 2;
        int objidx = 1;
        lua_getglobal(L,mt_name.c_str());                   //[4] = _G[mt_name]
        lua_pushstring(L,"__cglb_setters");                 //[5] = string
        lua_rawget(L,-2);                                   //[5] = mt_name[__cglb_setters]
        
        lua_pushvalue(L,keyidx);                            //[6] = key
        lua_rawget(L,-2);                                   //[6] = [5][key]
        //all setters are functions defined from class_luadef<T>
        if(lua_type(L,-1) == LUA_TFUNCTION)
        {
            lua_pushvalue(L,objidx);                        //[7] = obj
            lua_pushvalue(L,keyidx);                        //[8] = key
                                                            //[9+] = value(s)
            for(int i = keyidx + 1; i <= validx; ++i)
            {
                lua_pushvalue(L,i);
            }
            if(lua_pcall(L,validx - keyidx,0,0) != 0) 
                luaL_error(L,"%s.__newindex for %s",class_name.c_str(),lua_tostring(L,keyidx));
        }
        return 0;
    }

};

//invalid names for Lua, so this shouldn't clash 
template<typename T>
std::string class_luarep<T>::class_name = "@";
template<typename T>
std::string class_luarep<T>::mt_name = "@";

template<typename T>
void* class_luarep<T>::current_deleter = nullptr;

template<typename T>
void default_classrep_deleter(T* obj)
{
    delete obj;
    obj = NULL;
}

}
