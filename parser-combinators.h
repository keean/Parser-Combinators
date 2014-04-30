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
// File Stream With Location (Row/Col)

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
// Primitive String Recognisers

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
// Lifting String Recognisers to Parsers

template <typename Parser> class parser_push {
    Parser const p;

public:
    using is_parser_type = true_type;
    using result_type = vector<typename Parser::result_type>;

    explicit parser_push(Parser const& p) : p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        typename Parser::result_type tmp;
        if (p(in, &tmp)) {
            result->push_back(move(tmp));
            return true;
        }
        return false;
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_push<P> const push (P const& p) {
    return parser_push<P>(p);
}

//----------------------------------------------------------------------------

template <size_t> struct seq_idx {};

template <typename Parsers, typename Result_Type, size_t Index>
bool seq_apply(Parsers const &ps, Result_Type *result, fparse &in, seq_idx<Index>) {
    static constexpr int J = tuple_size<Parsers>::value - Index;
    if (get<J>(ps)(in, &get<J>(*result))) {
        return seq_apply(ps, result, in, seq_idx<Index - 1>());
    }
    return false;
}

template <typename Parsers, typename Result_Type> 
bool seq_apply(Parsers const &ps, Result_Type *result, fparse &in, seq_idx<1>) {
    static constexpr int J = tuple_size<Parsers>::value - 1;
    return get<J>(ps)(in, &get<J>(*result));
}

template <typename... Parsers> class parser_tuple {
    using tuple_type = tuple<Parsers...>;
    tuple_type const ps;

public:
    using is_parser_type = true_type;
    using result_type = tuple<typename Parsers::result_type...>;

    parser_tuple(Parsers const&... ps) : ps(ps...) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return seq_apply(ps, result, in, seq_idx<sizeof...(Parsers)>());
    }
};

template <typename... PS>
parser_tuple<PS...> const seq(PS const&... ps) {
    return parser_tuple<PS...>(ps...);
}

//----------------------------------------------------------------------------

template <typename Functor, typename Parser> class parser_fold {
    using functor_traits = function_traits<Functor>;
    Parser const p;
    Functor const f;

public:
    using is_parser_type = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

    parser_fold(Functor const& f, Parser const& p) 
        : f(f), p(p) {}

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

//----------------------------------------------------------------------------

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

//============================================================================
// Combinators can be used on both Parsers or Recognisers.

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

struct parser_fail {
    using is_parser_type = true_type;
    using result_type = void;

    explicit parser_fail() {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        return false;
    }
} const fail;

//----------------------------------------------------------------------------

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

template <typename Parser> class parser_option {
    Parser const p;

public:
    using is_parser_type = true_type;
    using result_type = typename Parser::result_type;

    explicit parser_option(Parser const& p) : p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        p(in, result);
        return true;
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_option<P> const option(P const& p) {
    return parser_option<P>(p);
}

//----------------------------------------------------------------------------

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

//----------------------------------------------------------------------------

template <typename Parser> class parser_many {
    Parser const p;

public:
    using is_parser_type = true_type;
    using result_type = typename Parser::result_type;

    explicit parser_many(Parser const& p) : p(p) {}

    template <typename Result_Type>
    bool operator() (fparse &in, Result_Type *result = nullptr) const {
        if (p(in, result)) {
            while (p(in, result));
            return true;
        }
        return false;
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_many<P> const many(P const& p) {
    return parser_many<P>(p);
}

