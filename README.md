Compiler Generated Lua Binding Library (CGLB)
============================
The C++ metatemplate programming based library for allowing Lua functions tocall your C++ code.

It takes inspiration from [luabind](http://www.rasterbar.com/products/luabind), which happens to be a much more feature-filled heavyweight library. If the basics are all that is necessary, then this library is for you.

Short example
--

```C++
struct Dog
{
    void Bark();
};

//Registering the Dog type for Lua
cglb::class_luadef<Dog>(L,"Dog")
    .constructor<>() //so that it can be constructed in Lua rather than just used
    .add("Bark",&Dog::Bark);


//Elsewhere in the C++ code, we wish to add another dog to lua
//as a global variable
Dog* dog = new Dog();
cglb::class_luarep<Dog>::push(L,dog,true);
lua_setglobal(L,"cppDog");

//Upon shutting down, after calling lua_close
cglb::class_luadef<Dog>::DeallocateLuaDefs();
```

In Lua code

```Lua
--Can create a new dog with a familiar C++ constructor syntax
local lassie = Dog()
lassie:Bark()
--And can use the global dog object we set earlier
cppDog:Bark()
```

How to use
---
The main interface is split in to two classes: `class_luarep` and `class_luadef`

`class_luarep` is used when dealing with C++ objects on the Lua stack, and
`class_luadef` is used when defining the interface to expose to Lua.

`T` is the type which is being defined in the following explainations

`class_luadef`:

Create an instance of a `class_luadef<T>` with the constructor
`class_luadef<T>(lua_State* L, const char* name)` where `L` is the `lua_State` in which you wish to define this type, and `name` is the name of the type on the Lua side of things. You can create as many of those objects as  you wish, but the use of two `class_luadef<T>` objects with the same `lua_State` is not thread safe, since all of the `add` functions manipulate the Lua stack.
Once you have a `class_luadef<T>` object, you can use the following methods on it.


`class_luadef<T>& class_luadef<T>::add(....)`:
There are a few overloads, each with their own restrictions.

`add<FunctionPtr>(const char* function_name, FunctionPtr func)` is for exposing object methods to Lua so that they will be called with the colon (:) syntax. See the Dog example above. The `FunctionPtr` template argument can be automatically deduced if there are no overloads, but if there are any overloads, then you will have to specify the function signature as the template argument. There is an example of this in the test directory where `std::queue` and `std::vector` are exposed to Lua.
There is no direct support for overloads, meaning you will have to have multiple functions with different names exposed to Lua if you wish to expose multiple overloads of a C++ function.
It is also possible to manipulate the Lua stack with your own function if more complex behavior is required. To do this, pass a function whose signature matches `int(*)(lua_State*)` or `int(*)(lua_State*,T*)`. If you use the second signature, then `T` is retreived as the first position on the Lua stack.

`add<MemberDataPtr>(const char* memberdataname, MemberDataPtr memdata)`,
`add_readonly<MemberDataPtr>(const char* mdname, MemberDataPtr mdat)`,
`add_writeonly<MemberDataPtr>(const char* mdname, MemberDataPtr mdat)`
Those functions are used for exposing a member data pointer to Lua in a way where they will be accessed with the dot (.) syntax.
`add` is the same as doing both `add_readonly` and `add_writeonly`, and as the names imply, `add_readonly` generates code which only allows reading the value of the variable, and `add_writeonly` generates code which only allows writing to the variable through the assignment operator.


`class_luadef<T>&  class_luadef<T>::destructor<FnPtr>(FnPtr func)` is used to define the function that is called when an instance of `T` in Lua is garbage collected. `func` cannot be a member function, and it must take only a single argument, which is a pointer to the object which should be deallocated (example `void(*)(T*)`). Not all C++ objects pushed to Lua are garbage collected, which is explained for `class_luarep<T>::push` below.
The default destructor just calls delete on the object. If that is the desired behavior, then there is no need to define this for `T`.


`class_luadef<T>& class_luadef<T>::constructor<...>()` is used to define the constructor to use when the Lua code uses the `T(...)` syntax to create a new `T` object for use in Lua. The template parameters define which constructor will be called. When using this, the allocation is done with `new`, so do not have a `destructor` defined which uses `free`. Use `custom_constructor` if you wish to use `malloc` and `free`.


`class_luadef<T>& class_luadef<T>::custom_constructor<FnPtr>(FnPtr func)` is a function which is similar to `constructor<...>`, except for it will call `func` rather than the constructor. `func` should return `T*`, or should manipulate the stack itself and push a `T` to Lua which is garbage collected. `func` cannot be a member function.


`class_luadef<T>& class_luadef<T>::op[X]<FnPtr>(FnPtr func)` where X is a Lua metamethod without the leading underscores and CamelCase. A full list is below

* opAdd
* opSub
* opUnm
* opMul
* opDiv
* opMod
* opPow
* opConcat
* opEq
* opLt
* opLe

I have included some convenience functions for defining operator-based functions in class_operator_wrapper.h. For example, if you wanted to expose a less than operator that was defined for `T` in C++ as an operator overload, it would look a little like

```C++
struct T
{
    bool operator<(int rhs);
};
//elsewhere in the code
class_luadef<T>(L,"T")
    .opLt(&class_operator_wrapper<T,int>);
```
Again, there is no explicit support for overloads, so if you have multiple operator overloads for a type, only one can be exposed to Lua as the operator.


`class_luadef<T>::DeallocateLuaDefs()` is to be called *after* the final lua_close. This will deallocate all memory which was previously allocated from the definition functions (add,constructor,etc.,).



`class_luarep<T>` has only static methods, and is used to interact with the Lua stack, and hold some static information about T, like the class name.


`int class_luarep<T>::push(lua_State* L, T* obj, bool gc)` pushes `obj` on to the Lua stack, and returns the index of `obj` on the stack. `gc` defaults to false, and defines if `obj` should have the destructor called upon garbage collection. If Lua is going to take ownership of `obj`, then set `gc` to true.

`T* class_luarep<T>::check(lua_State* L, int narg)` retrieves the item from the Lua stack at index `narg`, returning a `T*` or NULL on failure.


Quick reference:
===================
for `class_luadef<T>`, all functions return a `class_luaref<T>&` for easy chaining of definitions.
`class_luadef<T>(lua_State* L, const char* luaName)`
`add<FnPtrT>(const char* name, FnPtrT func)`
`add<MemDatPtrT>(const char* name, MemDatPtrT memdat)`
`add_readonly<MemDatPtrT>(const char* name, MemDatPtrT memdat)`
`add_writeonly<MemDatPtrT>(const char* name, MemDatPtrT memdat)`
`constructor<...>()`
`custom_constructor<FnPtrT>(FnPtrT func)`
`destructor<FnPtrT>(FnPtrT func)`
`opAdd<FnPtrT>(FnPtrT func)`
`opSub<FnPtrT>(FnPtrT func)`
`opUnm<FnPtrT>(FnPtrT func)`
`opMul<FnPtrT>(FnPtrT func)`
`opDiv<FnPtrT>(FnPtrT func)`
`opMod<FnPtrT>(FnPtrT func)`
`opPow<FnPtrT>(FnPtrT func)`
`opConcat<FnPtrT>(FnPtrT func)`
`opEq<FnPtrT>(FnPtrT func)`
`opLt<FnPtrT>(FnPtrT func)`
`opLe<FnPtrT>(FnPtrT func)`
`static void DeallocateLuaDefs()`

for `class_luarep<T>`, no constructor because it only uses static methods
`int push(lua_State* L, T* obj, bool gc)`
`T*  check(lua_State* L, int narg)`


