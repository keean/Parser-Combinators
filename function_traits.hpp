//----------------------------------------------------------------------------
// copyright 2014 Keean Schupke
// compile with -std=c++11 
// function-traits.h

#ifndef FUNCTION_TRAITS_HPP
#define FUNCTION_TRAITS_HPP

#include <tuple>

using namespace std;

//----------------------------------------------------------------------------
// traits for functions, function objects and lambdas

template <typename F> struct function_traits;

template <typename return_type, typename... arg_types>
struct function_traits<return_type(*)(arg_types...)>
    : public function_traits<return_type(arg_types...)> {};

template <typename R, typename... Args>
struct function_traits<R(Args...)> {
    static constexpr size_t arity = sizeof...(Args);
    using return_type = R;

    template <size_t N> struct argument {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename tuple_element<N, tuple<Args...>>::type;
    };
};

template <typename class_type, typename return_type, typename... arg_types>
struct function_traits<return_type(class_type::*)(arg_types...)>
    : public function_traits<return_type(class_type&, arg_types...)> {};

template <typename class_type, typename return_type, typename... arg_types>
struct function_traits<return_type(class_type::*)(arg_types...) const>
    : public function_traits<return_type(class_type&, arg_types...)> {};

template <typename class_type, typename return_type>
struct function_traits<return_type(class_type::*)>
    : public function_traits<return_type(class_type&)> {};

template <typename F> struct function_traits {
    using call_type = function_traits<decltype(&F::operator())>;
    using return_type = typename call_type::return_type;

    static constexpr size_t arity = call_type::arity - 1;
               
    template <size_t N>
    struct argument {
        static_assert(N < arity, "error: invalid parameter index.");
        using type = typename call_type::template argument<N+1>::type;
    };
};

#endif // FUNCTION_TRAITS_HPP
