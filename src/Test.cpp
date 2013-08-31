#include "Test.h"
#include <cglb/class_luadef.h>
#include <cglb/lua_include.h>
#include <fstream>
#include <vector>
#include <iostream>
#include "stl/lua_stl.h"
#include "stl/lua_stl_vector.h"

namespace cglb {
namespace test {
    

void Report(lua_State* L)
{
    const char* msg = lua_tostring(L,-1);
    std::string strmsg;

    while(msg)
    {
        lua_pop(L,1);
        strmsg.append(msg);
        
        std::cout << strmsg << std::endl;
        msg = lua_tostring(L,-1);
    }

}

#define DOLUASTRING(str) if(luaL_dostring(L,str) != 0) { Report(L); } 

struct RetStructTest
{
    int a;
};

struct TStruct
{
    TStruct() : mdat(0.0){}
    TStruct(double x, int a) : mdat(x){}
    ~TStruct()
    {
    }

    bool ValRetFunction(double x)
    {
        mdat += x;
        return true;
    }

    void NonReturningFunction()
    {
        mdat += 2.0;
    }

    RetStructTest* StructReturningFunction()
    {
        auto* ret = new RetStructTest();
        ret->a = 8;
        return ret;
    }

    void NotRegisteredFunction(int x)
    {
        mdat /= (double)x;
    }

    double mdat;
};

struct NonCopyStruct
{
    int s;
private:
    NonCopyStruct(NonCopyStruct const& copy);
};


bool TestRegClasses(lua_State* L);
bool TestMemberFunctionValRet(lua_State* L);
bool TestMemberFunctionNonValRet(lua_State* L);
bool TestMemberData(lua_State* L);
bool TestNonRegisteredMemberFunction(lua_State* L);
bool TestNonRegisteredMemberData(lua_State* L);
bool TestConstructor(lua_State* L);
bool TestVector(lua_State* L);
bool TestShutdown(lua_State* L);


template<typename T>
bool PushGlobalStruct(lua_State* L, T* obj, bool gc, const char* name);

void TestClass()
{
    std::cout << "Begining test." << std::endl;
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    if(!TestRegClasses(L))
        std::cout << "Failed registering classes" << std::endl;
    if(!TestMemberFunctionValRet(L))
        std::cout << "Failed member function value return" << std::endl;
    if(!TestMemberFunctionNonValRet(L))
        std::cout << "Failed member function non value returning function" << std::endl;
    if(!TestMemberData(L))
        std::cout << "Failed member data.";
    if(!TestNonRegisteredMemberFunction(L))
        std::cout << "Failed the non registered member function." << std::endl;
    if(!TestNonRegisteredMemberData(L))
        std::cout << "Failed the non registered member data." << std::endl;
    if(!TestConstructor(L))
        std::cout << "Failed constructor. " << std::endl;
    if(!TestVector(L))
        std::cout << "Failed vector." << std::endl;

    lua_close(L);
    //Want to deallocate the functions AFTER Lua has shutdown
    if(!TestShutdown(L))
        std::cout << "Failed shutdown." << std::endl;
}

template<typename T>
void TypeDestructor(T* obj)
{
    delete obj;
    obj = NULL;
}


bool TestRegClasses(lua_State* L)
{

    class_luadef<TStruct>(L,"TStruct")
        .add("ValRetFunction",&TStruct::ValRetFunction)
        .add("NonReturningFunction",&TStruct::NonReturningFunction)
        .add("StructReturningFunction",&TStruct::StructReturningFunction)
        .add("mdat",&TStruct::mdat)
        .destructor(&TypeDestructor<TStruct>)
        .constructor<double,int>();

    class_luadef<RetStructTest>(L,"RStruct")
        .add("a",&RetStructTest::a);

    stl::expose_nonconstvector<float>::type::Expose(L,"VectorFloat");

    return true;
}

bool TestShutdown(lua_State* L)
{
    typedef stl::expose_nonconstvector<float>::type::vecT VecT;
    class_luadef<VecT>::DeallocateLuaDefs();
    class_luadef<TStruct>::DeallocateLuaDefs();
    class_luadef<RetStructTest>::DeallocateLuaDefs();
    return true;
}


bool TestConstructor(lua_State* L)
{
    DOLUASTRING("test_construct = TStruct(15.0,3)");
    lua_getglobal(L,"test_construct");
    TStruct* t = class_luarep<TStruct>::check(L,-1);
    if(!t)
        return false;
    return true;
}

template<typename T>
bool PushGlobalStruct(lua_State* L, T* obj, bool gc, const char* name)
{
    class_luarep<T>::push(L,obj,gc);

    T* c = class_luarep<T>::check(L,-1);
    if(!c)
    {
        return false;
    }
    lua_setglobal(L,name);
    return true;
}


bool TestMemberFunctionValRet(lua_State* L)
{
    TStruct* t = new TStruct();
    t->mdat = 10.0;

    if(!PushGlobalStruct(L,t,true,"gtestStruct")) //gc==true, so don't delete it
    {
        delete t;
        return false;
    }

    DOLUASTRING("bval = gtestStruct:ValRetFunction(25.0)");
    lua_getglobal(L,"bval");
    std::string type = luaL_typename(L,-1);
    if(!lua_isboolean(L,-1) && bool(lua_toboolean(L,-1)) != true)
    {
        return false;
    }
    lua_pop(L,1);

    DOLUASTRING("retstruct = gtestStruct:StructReturningFunction()");
    lua_getglobal(L,"retstruct");
    RetStructTest* rst = class_luarep<RetStructTest>::check(L,-1);
    if(!rst)
        return false;

    return true;

}

bool TestMemberFunctionNonValRet(lua_State* L)
{
    TStruct* t = new TStruct();
    t->mdat = 20.0;

    if(!PushGlobalStruct(L,t,false,"nvfgTestStruct"))
    {
        delete t;
        return false;
    }

    DOLUASTRING("nvfgTestStruct:NonReturningFunction()");
    delete t;
    return true;

}


bool TestMemberData(lua_State* L)
{
    TStruct* t = new TStruct();
    t->mdat = 30.0;
    double first_res = 0.0;

    if(!PushGlobalStruct(L,t,false,"mdgTestStruct"))
        goto failure;

    DOLUASTRING("mdat_test = mdgTestStruct.mdat");
    lua_getglobal(L,"mdat_test");
    first_res = luaL_checknumber(L,-1);
    if(std::abs(first_res - 30.0) > 0.001)
        goto failure;
    if(std::abs(first_res - t->mdat) > 0.001)
        goto failure;

    goto success;

failure:
    delete t;
    return false;
success:
    delete t;
    return true;
}


bool TestNonRegisteredMemberFunction(lua_State* L)
{
    TStruct* t = new TStruct();
    t->mdat = 40.0;
    if(!PushGlobalStruct(L,t,false,"nrmfTestStruct"))
        goto failure;

    DOLUASTRING("local x = nrmfTestStruct:NotRegisteredFunction()");
    goto success;


failure:
    delete t;
    return false;
success:
    delete t;
    return true;
}


bool TestNonRegisteredMemberData(lua_State* L)
{
    TStruct* t = new TStruct();
    t->mdat = 50.0;
    if(!PushGlobalStruct(L,t,false,"nrmdTestStruct"))
        goto failure;

    DOLUASTRING("local nrmd = nrmdTestStruct.notRegisteredMemberData\n"
                    "nrmdTestStruct.notRegisteredMemberData = 5");

    goto success;

failure:
    delete t;
    return false;
success:
    delete t;
    return true;
}



bool TestVector(lua_State* L)
{
    typedef std::vector<float> vtype;
    vtype vec = vtype();
    vtype* lvec = nullptr;
    if(!PushGlobalStruct(L,&vec,false,"testVector"))
        goto failure;

    DOLUASTRING("testVector:push_back(0.0)")
    lua_getglobal(L,"testVector");
    lvec = class_luarep<vtype>::check(L,-1);
    if(!lvec)
        goto failure;
    if(lvec->empty())
        goto failure;
    if(lvec->at(0) != 0.0)
        goto failure;

    DOLUASTRING("testVector:push_back(1.0) testVector:push_back(2.0) testVector:push_back(3.0)");
    lua_getglobal(L,"testVector");
    lvec = class_luarep<vtype>::check(L,-1);
    if(!lvec)
        goto failure;
    if(lvec->empty())
        goto failure;
    if(lvec->size() != 4)
        goto failure;

    DOLUASTRING("local itrbegin = testVector:begin() \nlocal itrend = testVector:iend()\n \
        while itrbegin ~= itrend do\n \
            print(itrbegin:get())\n \
            itrbegin = itrbegin + 1\n \
        end ");
        
    goto success;

failure:
    return false;
success:
    return true;
}

}
}
