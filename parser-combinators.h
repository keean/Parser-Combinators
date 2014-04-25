//----------------------------------------------------------------------------
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// parser.h

#include <fstream>
#include <stdexcept>
#include <vector>
#include <tuple>
#include <type_traits>

using namespace std;

template <typename...> struct type_debug;

template <size_t> struct idx {};

template <typename T, size_t I> ostream& print_tuple(ostream& out, T const& t, idx<I>) {
    out << get<I>(t) << ", ";
    return print_tuple(out, t, idx<I - 1>());
}

template <typename T> ostream& print_tuple(ostream& out, T const& t, idx<0>) {
    return out << get<0>(t);
}

template <typename... T> ostream& operator<< (ostream& out, tuple<T...> const& t) {
    out << '<';
    print_tuple(out, t, idx<sizeof...(T) - 1>());
    return out << '>';
}

template <typename T> ostream& operator<< (ostream& out, vector<T> const& v) {
    out << "[";
    for (typename vector<T>::const_iterator i = v.begin(); i != v.end(); ++i) {
        cout << *i;
        typename vector<T>::const_iterator j = i;
        if (++j != v.end()) {
            cout << ", ";
        }
    }
    return out << "]";
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------

template <typename P1, typename P2> class is_either {
    P1 const p1;
    P2 const p2;

public:
    using is_predicate_type = true_type;
    string const name;
    is_either(P1&& p1, P2&& p2)
        : p1(forward<P1>(p1)), p2(forward<P2>(p2)), name("(" + p1.name + " or " + p2.name + ")") {}
    bool operator() (int const c) const {
        return p1(c) || p2(c);
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_predicate_type,
    typename = typename P2::is_predicate_type>
is_either<P1, P2> const operator|| (P1 p1, P2 p2) {
    return is_either<P1, P2>(move(p1), move(p2));
}

//----------------------------------------------------------------------------

template <typename P1> class is_not {
    P1 const p1;

public:
    using is_predicate_type = true_type;
    string const name;
    explicit is_not(P1&& p1) 
        : p1(forward<P1>(p1)), name("~" + p1.name) {}
    bool operator() (int const c) const {
        return !p1(c);
    }
};

template <typename P1,
    typename = typename P1::is_predicate_type>
is_not<P1> const operator~ (P1 p1) {
    return is_not<P1>(move(p1));
}

//----------------------------------------------------------------------------
// Recursive Descent Parser

struct parse_error : public runtime_error {
    int const row;
    int const col;
    int const sym;
    string const exp;
    parse_error(string const what, int const row, int const col, string const exp, int const sym)
        : runtime_error(move(what)), row(row), col(col), exp(move(exp)), sym(sym) {}
};

class fparse {
    fstream &in;
    int row;
    int col;
    int sym;

public:
    fparse(fstream &f) : in(f), row(1), col(1), sym(f.get()) {}

    void error(string const err, string const exp) {
        throw parse_error(move(err), row, col, move(exp), sym);
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

//----------------------------------------------------------------------------
    
template <typename P> class p_accept {
    P const pred;

public:
    using value_type = string;

    explicit p_accept(P&& p) : pred(forward<P>(p)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        int const sym = p.get_sym();
        if (!pred(sym)) {
            return false;
        }
        if (v != nullptr) {
            v->push_back(sym);
        }
        p.next();
        return true;
    }
};

template <typename P, typename = typename P::is_predicate_type>
p_accept<P> const accept(P p) {
    return p_accept<P>(move(p));
}

//----------------------------------------------------------------------------

template <typename P> class p_expect {
    P const pred;

public:
    using value_type = string;

    explicit p_expect(P&& p) : pred(forward<P>(p)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        int const sym = p.get_sym();
        if (!pred(sym)) {
            p.error("expected", pred.name);   
        }
        if (v != nullptr) {
            v->push_back(sym);
        }
        p.next();
        return true;
    }
};

template <typename P, typename = typename P::is_predicate_type>
p_expect<P> const expect(P p) {
    return p_expect<P>(move(p));
}

//----------------------------------------------------------------------------

template <typename P1> class p_option {
    P1 const p1;

public:
    using value_type = typename P1::value_type;

    explicit p_option(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        bool const t = p1(p, v);
        return true;
    }
};

template <typename P1, typename = typename P1::value_type>
p_option<P1> const option(P1 p1) {
    return p_option<P1>(move(p1));
}

//----------------------------------------------------------------------------

template <typename P1> class p_many {
    P1 const p1;

public:
    using value_type = typename P1::value_type;

    explicit p_many(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (p1(p, v)) {
            while (p1(p, v));
            return true;
        }
        return false;
    }
};

template <typename P1, typename = typename P1::value_type>
p_many<P1> const many(P1 p1) {
    return p_many<P1>(move(p1));
}

//----------------------------------------------------------------------------

template <typename P1, typename P2> class p_sepby {
    P1 const p1;
    P2 const p2;

public:
    using value_type = typename P1::value_type;

    explicit p_sepby(P1&& p1, P2&& p2) : p1(forward<P1>(p1)), p2(forward<P2>(p2)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (p1(p, v)) {
            while(p2(p, nullptr) && p1(p, v));
            return true;
        }
        return false;
    }
}; 

template <typename P1, typename P2,
    typename = typename P1::value_type,
    typename = typename P2::value_type>
p_sepby<P1, P2> const sepby(P1 p1, P2 p2) {
    return p_sepby<P1, P2>(move(p1), move(p2));
}

//----------------------------------------------------------------------------

template <typename P1, typename P2> class p_either { 
    P1 const p1;
    P2 const p2;

public:
    using value_type = typename P1::value_type;

    p_either(P1&& p1, P2&& p2) : p1(forward<P1>(p1)), p2(forward<P2>(p2)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        return p1(p, v) || p2(p, v);
    }
};

template <typename P1, typename P2,
    typename = typename P1::value_type,
    typename = typename P2::value_type,
    typename = typename enable_if<is_same<typename P1::value_type, typename P2::value_type>::value>::type>
p_either<P1, P2> const operator|| (P1 p1, P2 p2) {
    return p_either<P1, P2>(move(p1), move(p2));
}

//----------------------------------------------------------------------------

template <typename P1, typename P2> class p_sequence {
    P1 const p1;
    P2 const p2;

public:
    using value_type = typename P1::value_type;

    p_sequence(P1&& p1, P2&& p2) : p1(forward<P1>(p1)), p2(forward<P2>(p2)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        return p1(p, v) && p2(p, v);
    }
};

template <typename P1, typename P2,
    typename = typename P1::value_type,
    typename = typename P2::value_type,
    typename = typename enable_if<is_same<typename P1::value_type, typename P2::value_type>::value>::type>
p_sequence<P1, P2> const operator&& (P1 p1, P2 p2) {
    return p_sequence<P1, P2>(move(p1), move(p2));
}

//----------------------------------------------------------------------------

template <typename P1> class p_lift_vector {
    P1 const p1;

public:
    using value_type = vector<typename P1::value_type>;

    p_lift_vector(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (v == nullptr) {
            return p1(p, nullptr);
        }
        typename P1::value_type v1;
        if (p1(p, &v1)) {
            v->push_back(move(v1));
            return true;
        }
        return false;
    }
};

template <typename P1, typename = typename P1::value_type>
p_lift_vector<P1> const lift_vector(P1 p1) {
    return p_lift_vector<P1>(move(p1));
}

//----------------------------------------------------------------------------

template <typename P1> class p_lift_map {
    P1 const p1;

public:
    using value_type = map<typename P1::first_type, typename P1::second_type::value_type>;

    p_lift_map(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (v == nullptr) {
            return p1.second(p, nullptr);
        }
        if (v->find(p1.first) != v->end()) {
            p.error("map key already exists", "'" + p1.first + "'");
        }
        typename P1::second_type::value_type v1;
        if (p1.second(p, &v1)) {
            v->insert(make_pair(p1.first, v1));
            return true;
        }
        return false;
    }
};

template <typename P1, typename = typename P1::second_type::value_type>
p_lift_map<P1> const lift_map(P1 p1) {
    return p_lift_map<P1>(move(p1));
}

//----------------------------------------------------------------------------

template <typename F1, typename P1> class p_fold {
    P1 const p1;
    F1 f1;

public:
    using value_type = vector<typename P1::value_type>;

    p_fold(F1&& f1, P1&& p1) : f1(f1), p1(p1) {}

    bool operator() (fparse &p, value_type *v = nullptr) {
        if (v == nullptr) {
            if (p1(p, nullptr)) {
                while (p1(p, nullptr));
                return true;
            }
        } else {
            typename P1::value_type t;
            if (p1(p, &t)) {
                do {
                    f1(v, t);
                } while (p1(p, &t));
                return true;
            }
        }
        return false;
    }
};

template <template<typename> class F1, typename P1,
    typename = typename P1::value_type>
p_fold<F1<typename P1::value_type>, P1> const fold(P1 p1) {
    return p_fold<F1<typename P1::value_type>, P1>(move(F1<typename P1::value_type>()), move(p1));
}

//----------------------------------------------------------------------------

template <size_t I, typename P, typename V> struct apply_parsers {
    bool operator() (P const &p, V *v, fparse &in) {
        if (get<I>(p)(in, &get<I>(*v))) {
            return apply_parsers<I - 1, P, V>()(p, v, in);
        }
        return false;
    }
};

template <typename P, typename V> struct apply_parsers<0, P, V> {
    bool operator() (P const &p, V *v, fparse &in) {
        return get<0>(p)(in, &get<0>(*v));
    }
};

template <typename... PS> class p_seq {
    using T = tuple<PS...>;
    T const ps;

public:
    using value_type = tuple<typename PS::value_type...>;

    p_seq(PS&&... ps) : ps(forward<PS>(ps)...) {}

    bool operator() (fparse &in, value_type *v = nullptr) const {
        return apply_parsers<sizeof...(PS) - 1, T, value_type>()(ps, v, in);
    }
};

template <typename... PS>
p_seq<PS...> const seq(PS... ps...) {
    return p_seq<PS...>(move(ps)...);
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
            p1_type u;
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
            p1_type u;
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
    using value_type = typename F<true_type>::value_type;

    p_reduce(PS&&... ps) : ps(make_tuple(forward<PS>(ps)...)) {}

    bool operator() (fparse &in, value_type *v = nullptr) const {
        return apply_reduce<sizeof...(PS), F, parser_types>()(ps, v, in);
    }
};

template <template <typename> class F, typename... PS>
p_reduce<F, PS...> const reduce(PS... ps...) {
    return p_reduce<F, PS...>(move(ps)...);
}

//----------------------------------------------------------------------------

auto const is_minus = is_char('-');

auto const space = many(accept(is_space));
auto const number = many(accept(is_digit));
auto const signed_number = option(accept(is_minus)) && number;
auto const name = accept(is_alpha) && option(many(accept(is_alnum)));

struct parse_int {
    typedef int value_type;
    parse_int() {}
    bool operator() (fparse &p, value_type *i = nullptr) const {
        string s;
        if (signed_number(p, &s)) {
            if (i != nullptr) {
                *i = stoi(s);
            }
            return true;
        }
        return false;
    }
} const parse_int;

// polymorphic reduction function.
template <typename T> struct acc_int {
    using value_type = vector<int>;
    void operator() (value_type *a, T b) {}
};

template <> struct acc_int<int> {
    using value_type = vector<int>;
    void operator() (value_type *a, int b) {
        a->push_back(move(b));
    }
};

// example parser for CSV files, result is: vector<vector<int>>
auto reduce_int = many(lift_vector(many(reduce<acc_int>(parse_int, accept(is_char(',')) && space)) && reduce<acc_int>(space)));

