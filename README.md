Compiler Generated Lua Binding Library (CGLB)
============================
The C++ metatemplate programming based library for allowing Lua functions to call your C++ code.

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
````

In Lua code

```Lua
--Can create a new dog with a familiar C++ constructor syntax
local lassie = Dog()
lassie:Bark()
--And can use the global dog object we set earlier
cppDog:Bark()
````

How to use
---
(Also see the [Quick Reference](#quick-reference) at the bottom, which contains links to more detailed docs.)

The main interface is split in to two classes: `class_luarep` and `class_luadef`, plus an optional interface for `Init` and `Quit`, which will remove some boring and repetitive code, but uses even more global state. See [`Init and Quit`](#init-and-quit).

`class_luarep` is used when dealing with C++ objects on the Lua stack, and

`class_luadef` is used when defining the interface to expose to Lua.

`T` is the type which is being defined in the following explainations.

### `class_luadef<T>`:
In `class_luadef.h`

Create an instance of a `class_luadef<T>` with the constructor
`class_luadef<T>(lua_State* L, const char* name)` where `L` is the `lua_State` in which you wish to define this type, and `name` is the name of the type on the Lua side of things. You can create as many of those objects as  you wish, but the use of two `class_luadef<T>` objects with the same `lua_State` is not thread safe, since all of the `add` functions manipulate the Lua stack.
Once you have a `class_luadef<T>` object, you can use the following methods on it.


#### `class_luadef<T>& class_luadef<T>::add(....)`:
There are a few overloads, each with their own restrictions.

###### `add<FunctionPtr>(const char* function_name, FunctionPtr func)` 
is for exposing object methods to Lua so that they will be called with the colon (:) syntax. See the Dog example above. The `FunctionPtr` template argument can be automatically deduced if there are no overloads, but if there are any overloads, then you will have to specify the function signature as the template argument. There is an example of this in the test directory where `std::queue` and `std::vector` are exposed to Lua.

There is no direct support for overloads, meaning you will have to have multiple functions with different names exposed to Lua if you wish to expose multiple overloads of a C++ function.
It is also possible to manipulate the Lua stack with your own function if more complex behavior is required. To do this, pass a function whose signature matches `int(*)(lua_State*)` or `int(*)(lua_State*,T*)`. If you use the second signature, then `T` is retreived as the first position on the Lua stack.



###### `add<MemberDataPtr>(const char* memberdataname, MemberDataPtr memdata)`,
###### `add_readonly<MemberDataPtr>(const char* mdname, MemberDataPtr mdat)`,
###### `add_writeonly<MemberDataPtr>(const char* mdname, MemberDataPtr mdat)`
Those functions are used for exposing a member data pointer to Lua in a way where they will be accessed with the dot (.) syntax.

`add` is the same as doing both `add_readonly` and `add_writeonly`, and as the names imply, `add_readonly` generates code which only allows reading the value of the variable, and `add_writeonly` generates code which only allows writing to the variable through the assignment operator.



#### `class_luadef<T>&  class_luadef<T>::destructor<FnPtr>(FnPtr func)`
is used to define the function that is called when an instance of `T` in Lua is garbage collected. `func` cannot be a member function, and it must take only a single argument, which is a pointer to the object which should be deallocated (example `void(*)(T*)`). Not all C++ objects pushed to Lua are garbage collected, which is explained for `class_luarep<T>::push` below.

The default destructor just calls delete on the object. If that is the desired behavior, then there is no need to define this for `T`.


#### `class_luadef<T>& class_luadef<T>::constructor<...>()` 
is used to define the constructor to use when the Lua code uses the `T(...)` syntax to create a new `T` object for use in Lua. The template parameters define which constructor will be called. When using this, the allocation is done with `new`, so do not have a `destructor` defined which uses `free`. Use `custom_constructor` if you wish to use `malloc` and `free`.


#### `class_luadef<T>& class_luadef<T>::custom_constructor<FnPtr>(FnPtr func)` 
is a function which is similar to `constructor<...>`, except for it will call `func` rather than the constructor. `func` should return `T*`, or should manipulate the stack itself and push a `T` to Lua which is garbage collected. `func` cannot be a member function.


#### `class_luadef<T>& class_luadef<T>::op[X]<FnPtr>(FnPtr func)` 
where X is a Lua metamethod without the leading underscores and CamelCase. A full list is below

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
* opCall
* opLen

This library has included some convenience functions for defining operator-based functions in class_operator_wrapper.h. For example, if you wanted to expose a less than operator that was defined for `T` in C++ as an operator overload, it would look a little like

```C++
struct T
{
    bool operator<(int rhs);
};
//elsewhere in the code
class_luadef<T>(L,"T")
    .opLt(&class_operator_wrapper::less<T,int>);
````
Again, there is no explicit support for overloads, so if you have multiple operator overloads for a type, only one can be exposed to Lua as the operator.


#### `class_luadef<T>::DeallocateLuaDefs()` 
is to be called *after* the final lua_close. This will deallocate all memory which was previously allocated from the definition functions (add,constructor,etc.,).

#### `class_luadef<T>& class_luadef<T>::inherit<ParentT>`
Registers the same functions that `ParentT` registered, and will call them with `T` as `this`. It would be a good idea to have this as the first call if it inherits from a type, so that `T` can override any of the functions defined in `ParentT`.






### `class_luarep<T>`
In `class_luarep.h`

Has only static methods, and is used to interact with the Lua stack, and hold some static information about T, like the class name.


#### `int class_luarep<T>::push(lua_State* L, T* obj, bool gc)` 
Pushes `obj` on to the Lua stack, and returns the index of `obj` on the stack. `gc` defaults to false, and expresses if `obj` should have the destructor called upon garbage collection. If Lua is going to take ownership of `obj`, then set `gc` to true.

#### `T* class_luarep<T>::check(lua_State* L, int narg)` 
Retrieves the item from the Lua stack at index `narg`, returning a `T*` or NULL on failure.

#### `int class_luarep<T>::tostring(lua_State* L)`
Pushes a string on to the stack which represents the address of the object. Can override, and it expects a string object on the top of the stack. Called on the __tostring metamethod.
```
Stackstate on call:
[1] = T** userdata (self)

Stackstate after call:
[-1] = string
````

#### `int class_luarep<T>::index(lua_State* L)`
Called on the `__index` metamethod. Default implementation checks the private `__cglb_getters` to see if it is a memeber data access, and if not, then it will check the type's `__index` field. If you create an `__index` handler through `cglb::class_luadef<T>::opMeta("__index",func)`, it would be a good idea to call this default function if your `__index` lookup fails.
```
Stackstate on call:
[1] = T** userdata (self)
[2] = key

Stackstate after call:
[-n to -1] = values (where n is more than likely 1)
````


#### `int class_luarep<T>::newindex(lua_State* L)`
Called on the `__newindex` metamethod. Default implementation checks the private `__cglb_setters` to see if it is a member data access/assignment. Default does not check for an additional `__newindex` metamethod, unlike how `class_luarep<T>::index` does for `__index`. If you create a `__newindex` handler through `cglb::class_luadef<T>::opMeta("__newindex",func)`, it would be a good idea to call this default function if your `__newindex` lookup fails.
```
Stackstate on call:
[1] = T** userdata (self)
[2] = key
[3] = value

Stackstate after call:
__newindex returns nothing
````

#### `std::string class_luarep<T>::class_name`
#### `std::string class_luarep<T>::mt_name`
`class_name` and `mt_name` are associated upon the first constructor of a `class_luadef<T>` object. `mt_name` is `class_name` appended with `_mt`, for example a class named `Person` would have a `class_name` = "Person" and a `mt_name` = "Person_mt".

`class_name` is what is used in Lua code in a constructor-like syntax, rather than having to do something similar to `classname:new`.


### Init and Quit

In `cglb_init.h`.

Removes the need to keep track of which types are registered by calling each type's `DeallocateLuaDefs` individually by hooking in to the type registration and making sure that all of the `DeallocateLuaDefs` calls are made upon the call to `cglb::Quit()`. This will only be tracked for types after `cglb::Init()` is called, so if you wish for this to happen for all types, then call `cglb::Init()` before any of the `class_luadef<T>` constructors are called.

If this is not used, then `class_luadef<T>::DeallocateLuaDefs` will have to be called for each individual type at the end of the program if you wish for all of the memory allocated by this library to be released.


Quick reference:
===================
for [`class_luadef<T>`](#class_luadeft), all functions return a `class_luaref<T>&` for easy chaining of definitions.

- [`class_luadef<T>(lua_State* L, const char* luaName)`] (#class_luadeft)
- [`inherit<ParentT>()`]                        (#class_luadeft-class_luadeftinheritparentt)
- [`add<FnPtrT>(const char* name, FnPtrT func)`] (#addfunctionptrconst-char-function_name-functionptr-func)
- [`add<MemDatPtrT>(const char* name, MemDatPtrT memdat)`] (#addmemberdataptrconst-char-memberdataname-memberdataptr-memdata)
- [`add_readonly<MemDatPtrT>(const char* name, MemDatPtrT memdat)`] (#add_readonlymemberdataptrconst-char-mdname-memberdataptr-mdat)
- [`add_writeonly<MemDatPtrT>(const char* name, MemDatPtrT memdat)`] (#add_writeonlymemberdataptrconst-char-mdname-memberdataptr-mdat)
- [`constructor<...>()`]                        (#class_luadeft-class_luadeftconstructor)
- [`custom_constructor<FnPtrT>(FnPtrT func)`]   (#class_luadeft-class_luadeftcustom_constructorfnptrfnptr-func)
- [`destructor<FnPtrT>(FnPtrT func)`]           (#class_luadeft--class_luadeftdestructorfnptrfnptr-func)
- [`opAdd<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opSub<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opUnm<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opMul<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opDiv<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opMod<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opPow<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opConcat<FnPtrT>(FnPtrT func)`]             (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opEq<FnPtrT>(FnPtrT func)`]                 (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opLt<FnPtrT>(FnPtrT func)`]                 (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opLe<FnPtrT>(FnPtrT func)`]                 (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opCall<FnPtrT>(FnPtrT func)`]               (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`opLen<FnPtrT>(FnPtrT func)`]                (#class_luadeft-class_luadeftopxfnptrfnptr-func)
- [`static void DeallocateLuaDefs()`]           (#class_luadeftdeallocateluadefs)

for [`class_luarep<T>`](#class_luarept), no constructor because it only uses static methods
- [`int push(lua_State* L, T* obj, bool gc)`]   (#int-class_luareptpushlua_state-l-t-obj-bool-gc)
- [`T*  check(lua_State* L, int narg)`]         (#t-class_luareptchecklua_state-l-int-narg)
- [`int tostring(lua_State* L)`]                (#int-class_luarepttostringlua_state-l)
- [`int index(lua_State* L)`]                   (#int-class_luareptindexlua_state-l)
- [`int newindex(lua_State* L)`]                (#int-class_luareptnewindexlua_state-l)

[`Init` and `Quit` global functions]            (#init-and-quit)
