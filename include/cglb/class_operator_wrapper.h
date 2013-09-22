#pragma once
/* 
 * Copyright (c) 2013 Nathan Starkey MIT License
 * See either the LICENSE file in the repo, or http://opensource.org/licenses/MIT 
 */
#include <functional>

namespace cglb 
{

/**
 * For use with class_luadef<T>::op[X] where X is one of the metamethods
 * ie opAdd(class_operator_wrapper::add<T,int>) would be appropriate to define
 * an add operation on an iterator to advance it forward.
 */
struct class_operator_wrapper
{
    template<typename operandType>
    static auto dereference(operandType obj) -> decltype(*obj)
    {
        return *obj;
    }

    template<typename leftType, typename rightType>
    static auto add(leftType left, rightType right) -> decltype(left + right)
    {
        return left + right;
    }

    template<typename leftType, typename rightType>
    static auto sub(leftType left, rightType right) -> decltype(left - right)
    {
        return left - right;
    }

    template<typename leftType, typename rightType>
    static auto mul(leftType left, rightType right) -> decltype(left * right)
    {
        return left * right;
    }

    template<typename leftType, typename rightType>
    static auto divide(leftType left, rightType right) -> decltype(left / right)
    {
        return left / right;
    }

    template<typename leftType, typename rightType>
    static auto equal(leftType left, rightType right) -> decltype(left == right)
    {
        return left == right;
    }

    template<typename leftType, typename rightType>
    static auto less(leftType left, rightType right) -> decltype(left < right)
    {
        return left < right;
    }

    template<typename leftType, typename rightType>
    static auto less_equal(leftType left, rightType right) -> decltype(left <= right)
    {
        return left <= right;
    }


    template<typename opType>
    static auto postinc(opType self) -> decltype(self++)
    {
        return self++;
    }

    template<typename opType>
    static auto predec(opType self) -> decltype(--self)
    {
        return --self;
    }

    template<typename opType>
    static auto postdec(opType self) -> decltype(self--)
    {
        return self--;
    }

    template<typename opType>
    static auto preinc(opType self) -> decltype(++self)
    {
        return ++self;
    }

};
}
