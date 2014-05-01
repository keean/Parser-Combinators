//============================================================================
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// parser.h

//#include <fstream>
#include <istream>
#include <stdexcept>
#include <vector>
#include <tuple>
#include <type_traits>
#include "function-traits.h"

using namespace std;

//============================================================================
// Character Predicates

struct is_any {
    using is_predicate_type = true_type;
    string const name = "anything";
    is_any() {}
    bool operator() (int const c) const {
        return c != EOF;
    }
} const is_any;

struct is_space {
    using is_predicate_type = true_type;
    string const name = "space";
    is_space() {}
    bool operator() (int const c) const {
        return ::isspace(c) != 0;
    }
} const is_space;

struct is_digit {
    using is_predicate_type = true_type;
    string const name = "digit";
    is_digit() {}
    bool operator() (int const c) const {
        return ::isdigit(c) != 0;
    }
} const is_digit;

struct is_upper {
    using is_predicate_type = true_type;
    string const name = "uppercase";
    is_upper() {}
    bool operator() (int const c) const {
        return ::isupper(c) != 0;
    }
} const is_upper;

struct is_lower {
    using is_predicate_type = true_type;
    string const name = "lowercase";
    is_lower() {}
    bool operator() (int const c) const {
        return ::islower(c) != 0;
    }
} const is_lower;

struct is_alpha {
    using is_predicate_type = true_type;
    string const name = "alphabetic";
    is_alpha() {}
    bool operator() (int const c) const {
        return ::isalpha(c) != 0;
    }
} const is_alpha;

struct is_alnum {
    using is_predicate_type = true_type;
    string const name = "alphanumeric";
    is_alnum() {}
    bool operator() (int const c) const {
        return ::isalnum(c) != 0;
    }
} const is_alnum;

struct is_print {
    using is_predicate_type = true_type;
    string const name = "printable";
    is_print() {}
    bool operator() (int const c) const {
        return ::isprint(c) != 0;
    }
} const is_print;

class is_char {
    int const k;

public:
    using is_predicate_type = true_type;
    string const name;
    explicit is_char(char const c)
        : k(c), name("'" + string(1, c) + "'") {}
    bool operator() (int const c) const {
        return k == c;
    }
};

is_char const is_eof(EOF);

//----------------------------------------------------------------------------
// Combining character predicates.

template <typename P1, typename P2> class is_either {
    P1 const p1;
    P2 const p2;

public:
    using is_predicate_type = true_type;
    string const name;
    is_either(P1 const& p1, P2 const& p2)
        : p1(p1), p2(p2), name("(" + p1.name + " or " + p2.name + ")") {}
    bool operator() (int const c) const {
        return p1(c) || p2(c);
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_predicate_type,
    typename = typename P2::is_predicate_type>
is_either<P1, P2> const operator|| (P1 const& p1, P2 const& p2) {
    return is_either<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------

template <typename P1> class is_not {
    P1 const p1;

public:
    using is_predicate_type = true_type;
    string const name;
    explicit is_not(P1 const& p1) 
        : p1(p1), name("~" + p1.name) {}
    bool operator() (int const c) const {
        return !p1(c);
    }
};

template <typename P1,
    typename = typename P1::is_predicate_type>
is_not<P1> const operator~ (P1 const& p1) {
    return is_not<P1>(p1);
}

//===========================================================================
// File Stream With Location (Row/Col) and Exceptions

struct parse_error : public runtime_error {
    int const row;
    int const col;
    int const sym;
    string const exp;
    parse_error(string const& what, int row, int col, string const& exp, int sym)
        : runtime_error(what), row(row), col(col), exp(exp), sym(sym) {}
};

class fparse {
    istream &in;
    int row;
    int col;
    int sym;

public:
    fparse(istream &f) : in(f), row(1), col(1), sym(f.get()) {}

    void error(string const& err, string const& exp) {
        throw parse_error(err, row, col, exp, sym);
    }

    void next() {
        sym = in.get();
        if (sym == '\n') {
            ++row;
            col = 0;
        } else if (::isprint(sym)) {
            ++col;
        }
    }

    int get_col() {
        return col;
    }
    
    int get_row() {
        return row;
    }

    int get_sym() {
        return sym;
    }
};

//============================================================================
// Type Helpers

//----------------------------------------------------------------------------
// Make tuples of integer sequences from begining to end - 1 of range
template <size_t... Is> struct size_sequence {};
template <size_t Begin, size_t End, size_t... Is> struct range : range<Begin, End - 1, End - 1, Is...> {};
template <size_t Begin, size_t... Is> struct range<Begin, Begin, Is...> {
    using type = size_sequence<Is...>;
};

//----------------------------------------------------------------------------
// Can a pointer to one type be implicitly converted into a pointer to the other.
template <typename A, typename B> struct is_compatible {
    static constexpr bool value = is_convertible<
            typename add_pointer<A>::type, typename add_pointer<B>::type
        >::value || is_convertible<
            typename add_pointer<B>::type, typename add_pointer<A>::type
        >::value;
};

//----------------------------------------------------------------------------
// Choose the result type of two parsers which a pointer the other result type
// can be conveted into a pointer to.  For example given a parsers with result types
// void and int, the least general is int, because (void*) can be implicitely converted
// to (int*). If there is no possible implicit converstion result_type is undefined.
template <typename P1, typename P2, typename = void> struct least_general {};

template <typename P1, typename P2> struct least_general <P1, P2, typename enable_if<is_convertible<
    typename add_pointer<typename P2::result_type>::type, 
    typename add_pointer<typename P1::result_type>::type
>::value && !is_same<typename P1::result_type, typename P2::result_type>::value>::type> {
    using result_type = typename P2::result_type;
};

template <typename P1, typename P2> struct least_general <P1, P2, typename enable_if<is_convertible<
    typename add_pointer<typename P1::result_type>::type, 
    typename add_pointer<typename P2::result_type>::type
>::value || is_same<typename P1::result_type, typename P2::result_type>::value>::type> {
    using result_type = typename P1::result_type;
};

//============================================================================
// Primitive String Recognisers: accept, expect

//----------------------------------------------------------------------------
// Stream is advanced if symbol matches, and symbol is appended to result.

template <typename Predicate> class parser_accept {
    Predicate const p;

public:
    using is_parser_type = true_type;
    using result_type = string;

    explicit parser_accept(Predicate const& p) : p(p) {}

    bool operator() (fparse &in, string *result = nullptr) const {
        int const sym = in.get_sym();
        if (!p(sym)) {
            return false;
        }
        if (result != nullptr) {
            result->push_back(sym);
        }
        in.next();
        return true;
    }
};

template <typename P, typename = typename P::is_predicate_type>
parser_accept<P> const accept(P const &p) {
    return parser_accept<P>(p);
}

//----------------------------------------------------------------------------
// Throw exception if symbol does not match, accept otherwise.

template <typename Predicate> class parser_expect {
    Predicate const p;

public:
    using is_parser_type = true_type;
    using result_type = string;

    explicit parser_expect(Predicate const& p) : p(p) {}

    bool operator() (fparse &in, string *result = nullptr) const {
        int const sym = in.get_sym();
        if (!p(sym)) {
            in.error("expected", p.name);
        }
        if (result != nullptr) {
            result->push_back(sym);
        }
        in.next();
        return true;
    }
};

template <typename P, typename = typename P::is_predicate_type>
parser_expect<P> const expect(P const& p) {
    return parser_expect<P>(p);
}

//============================================================================
// Constant Parsers: succ, fail

//----------------------------------------------------------------------------
// Always succeeds.

struct parser_succ {
    using is_parser_type = true_type;
    using result_type = void;

    explicit parser_succ() {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return true;
    }
} const succ;

//----------------------------------------------------------------------------
// Always fails.

struct parser_fail {
    using is_parser_type = true_type;
    using result_type = void;

    explicit parser_fail() {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return false;
    }
} const fail;

//============================================================================
// Lifting String Recognisers to Parsers: lift, map, fold

//----------------------------------------------------------------------------
// depricate single argument version of fmap in facour of variadic one.
/*
template <typename Functor, typename Parser> class parser_fmap {
    using functor_traits = function_traits<Functor>;
    Parser const p;
    Functor const f;

public:
    using is_parser_type = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

    explicit parser_fmap(Functor const& f, Parser const& p) : f(f), p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        typename Parser::result_type tmp {};
        if (p(in, &tmp)) {
            f(result, tmp);
            return true;
        }
        return false;
    }
};

template <typename F, typename P, typename = typename P::is_parser_type>
parser_fmap<F, P> const fmap (F const& f, P const& p) {
    return parser_fmap<F, P>(f, p);
}
*/
//----------------------------------------------------------------------------
// fold, depricate in favour of fmap, note fold = some(fmap(...))
/*
template <typename Functor, typename Parser> class parser_fold {
    using functor_traits = function_traits<Functor>;
    Parser const p;
    Functor const f;

public:
    using is_parser_type = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

    parser_fold(Functor const& f, Parser const& p) : f(f), p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        if (result == nullptr) {
            typename Parser::result_type* discard_result = nullptr;
            if (p(in, discard_result)) {
                while (p(in, discard_result));
                return true;
            }
            return false;
        }

        {
            typename Parser::result_type tmp {};
            if (!p(in, &tmp)) {
                return false;
            }
            f(result, tmp);
        }

        while (true) {
            typename Parser::result_type tmp {};
            if (!p(in, &tmp)) {
                return true;
            }
            f(result, tmp);
        }
    }
};

template <typename F, typename P,
    typename = typename P::is_parser_type,
    typename = typename enable_if<!is_same<typename P::result_type, void>::value>::type>
parser_fold<F, P> const fold(F const& f, P const& p) {
    return parser_fold<F, P>(f, p);
}
*/
//----------------------------------------------------------------------------
// If all parsers succeed, pass all results as arguments to user supplied functor

template <typename Functor, typename... Parsers> class parser_fmap {
    using functor_traits = function_traits<Functor>;
    using tuple_type = tuple<Parsers...>;
    using tmp_type = tuple<typename Parsers::result_type...>;
    tuple_type const ps;
    Functor const f;

    template <typename Rs, size_t I0, size_t... Is> 
    bool all_parsers(fparse &in, Rs &rs, size_t, size_t is...) const {
        if (get<I0>(ps)(in, &get<I0>(rs))) {
            return all_parsers<Rs, Is...>(in, rs, is);
        }
        return false;
    }

    template <typename Rs, size_t I0>
    bool all_parsers(fparse &in, Rs &rs, size_t) const {
        return get<I0>(ps)(in, &get<I0>(rs));
    }

    template <typename Result_Type, size_t... I> 
    bool fmap_all(fparse &in, size_sequence<I...> seq, Result_Type *result) const {
        tmp_type tmp {};
        if (all_parsers<tmp_type, I...>(in, tmp, I...)) {
            f(result, get<I>(tmp)...);
            return true;
        }
        return false;
    }

public:
    using is_parser_type = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

    parser_fmap(Functor const& f, Parsers const&... ps) : f(f), ps(ps...) {}

    template <typename Result_Type, typename Is = typename range<0, sizeof...(Parsers)>::type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return fmap_all(in, Is(), result);
    }
};

template <typename F, typename... PS>
parser_fmap<F, PS...> const fmap(F const& f, PS const&... ps) {
    return parser_fmap<F, PS...>(f, ps...);
}

//----------------------------------------------------------------------------
// polymorphic fold/reduce: dericate in favour of fmap, equivalent to fmap(...)
/*
template <size_t I, template <typename> class F, typename PS> struct apply_reduce {
    static constexpr int J = tuple_size<PS>::value - I;
    using p1_type = typename tuple_element<J, PS>::type::value_type;
    using f0_type = typename F<p1_type>::value_type;
    bool operator() (PS const &ps, f0_type *v, fparse &in) {
        if (v == nullptr) {
            if (get<J>(ps)(in, nullptr)) {
                return apply_reduce<I - 1, F, PS>()(ps, v, in);
            }
        } else {
            p1_type u {};
            if (get<J>(ps)(in, &u)) {
                F<p1_type> f;
                f(v, u);
                return apply_reduce<I - 1, F, PS>()(ps, v, in);
            }
        }
        return false;
    }
};

template <template <typename> class F, typename PS> struct apply_reduce<1, F, PS> {
    static constexpr int J = tuple_size<PS>::value - 1;
    using p1_type = typename tuple_element<J, PS>::type::value_type;
    using f0_type = typename F<p1_type>::value_type;
    bool operator() (PS const &ps, f0_type *v, fparse &in) {
        if (v == nullptr) {
            if (get<J>(ps)(in, nullptr)) {
                return true;
            }
        } else {
            p1_type u {};
            if (get<J>(ps)(in, &u)) {
                F<p1_type> f;
                f(v, u);
                return true;
            }
        }
        return false;
    }
};

template <template <typename> class F , typename... PS> class p_reduce {
    using parser_types = tuple<PS...>;
    parser_types const ps;

public:
    using value_type = typename F<typename tuple_element<0, parser_types>::type::value_type>::value_type;

    p_reduce(PS const&... ps) : ps(make_tuple(ps...)) {}

    bool operator() (fparse &in, value_type *v = nullptr) const {
        return apply_reduce<sizeof...(PS), F, parser_types>()(ps, v, in);
    }
};

template <template <typename> class F, typename... PS>
p_reduce<F, PS...> const reduce(PS const&... ps) {
    return p_reduce<F, PS...>(ps...);
}
*/
//============================================================================
// Combinators For Both Parsers and Recognisers: ||, &&, many, discard 

//----------------------------------------------------------------------------
// Run the second parser only if the first fails.

template <typename Parser1, typename Parser2> class parser_choice { 
    Parser1 const p1;
    Parser2 const p2;

public:
    using is_parser_type = true_type;
    using result_type = typename least_general<Parser1, Parser2>::result_type;

    parser_choice(Parser1 const& p1, Parser2 const& p2)
        : p1(p1), p2(p2) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return p1(in, result) || p2(in, result);
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_parser_type,
    typename = typename P2::is_parser_type,
    typename = typename enable_if<is_compatible<typename P1::result_type, typename P2::result_type>::value>::type>
parser_choice<P1, P2> const operator|| (P1 const& p1, P2 const& p2) {
    return parser_choice<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------
// Run the second parser only if the first succeeds. 

template <typename Parser1, typename Parser2> class parser_sequence {
    Parser1 const p1;
    Parser2 const p2;

public:
    using is_parser_type = true_type;
    using result_type = typename least_general<Parser1, Parser2>::result_type;

    parser_sequence(Parser1 const& p1, Parser2 const& p2)
        : p1(p1), p2(p2) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return p1(in, result) && p2(in, result);
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_parser_type,
    typename = typename P2::is_parser_type,
    typename = typename enable_if<is_compatible<typename P1::result_type, typename P2::result_type>::value>::type>
parser_sequence<P1, P2> const operator&& (P1 const& p1, P2 const& p2) {
    return parser_sequence<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------
// Accept the parser zero or more times.

template <typename Parser> class parser_many {
    Parser const p;

public:
    using is_parser_type = true_type;
    using result_type = typename Parser::result_type;

    explicit parser_many(Parser const& p) : p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        while (p(in, result));
        return true;
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_many<P> const many(P const& p) {
    return parser_many<P>(p);
}

//----------------------------------------------------------------------------
// Discard the result of the parser (and the result type), keep succeed or fail.

template <typename Parser> class parser_discard {
    Parser const p;

public:
    using is_parser_type = true_type;
    using result_type = void;

    explicit parser_discard(Parser const& p) : p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        typename Parser::result_type *const discard_result = nullptr;
        return p(in, discard_result);
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_discard<P> const discard(P const& p) {
    return parser_discard<P>(p);
}

//============================================================================
// Some derived definitions for convenience: option, some

//----------------------------------------------------------------------------
// Optionally accept the parser once.

template <typename P> auto option(P const& p) -> decltype(p || succ) {
    return p || succ;
}

//----------------------------------------------------------------------------
// Accept the parser one or more times.

template <typename P> auto some (P const& p) -> decltype(p && many(p)) {
    return p && many(p);
}

