//----------------------------------------------------------------------------
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// parser.h

#include <fstream>
#include <stdexcept>

using namespace std;

//----------------------------------------------------------------------------
// Character Predicates

struct is_any {
    typedef true_type is_predicate_type;
    string const name = "anything";
    is_any() {}
    bool operator() (int const c) const {
        return c != EOF;
    }
} const is_any;

struct is_space {
    typedef true_type is_predicate_type;
    string const name = "space";
    is_space() {}
    bool operator() (int const c) const {
        return ::isspace(c) != 0;
    }
} const is_space;

struct is_digit {
    typedef true_type is_predicate_type;
    string const name = "digit";
    is_digit() {}
    bool operator() (int const c) const {
        return ::isdigit(c) != 0;
    }
} const is_digit;

struct is_upper {
    typedef true_type is_predicate_type;
    string const name = "uppercase";
    is_upper() {}
    bool operator() (int const c) const {
        return ::isupper(c) != 0;
    }
} const is_upper;

struct is_lower {
    typedef true_type is_predicate_type;
    string const name = "lowercase";
    is_lower() {}
    bool operator() (int const c) const {
        return ::islower(c) != 0;
    }
} const is_lower;

struct is_alpha {
    typedef true_type is_predicate_type;
    string const name = "alphabetic";
    is_alpha() {}
    bool operator() (int const c) const {
        return ::isalpha(c) != 0;
    }
} const is_alpha;

struct is_alnum {
    typedef true_type is_predicate_type;
    string const name = "alphanumeric";
    is_alnum() {}
    bool operator() (int const c) const {
        return ::isalnum(c) != 0;
    }
} const is_alnum;

struct is_print {
    typedef true_type is_predicate_type;
    string const name = "printable";
    is_print() {}
    bool operator() (int const c) const {
        return ::isprint(c) != 0;
    }
} const is_print;

class is_char {
    int const k;

public:
    typedef true_type is_predicate_type;
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
    typedef true_type is_predicate_type;
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
    typedef true_type is_predicate_type;
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
            col = 1;
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
    typedef string value_type;

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
    typedef string value_type;

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
    typedef typename P1::value_type value_type;

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
    typedef typename P1::value_type value_type;

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
    typedef typename P1::value_type value_type;

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
    typedef typename P1::value_type value_type;

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
    typedef typename P1::value_type value_type;

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
    typedef vector<typename P1::value_type> value_type;

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
    typedef map<typename P1::first_type, typename P1::second_type::value_type> value_type;

    p_lift_map(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (v == nullptr) {
            return p1.second(p, nullptr);
        }
        typename P1::second_type::value_type v1;
        if (p1.second(p, &v1)) {
            if (v->insert(make_pair(p1.first, move(v1))).second) {
                return true;
            } else {
                p.error("map key already exists", p1.first);
            }
        }
        return false;
    }
};

template <typename P1, typename = typename P1::second_type::value_type>
p_lift_map<P1> const lift_map(P1 p1) {
    return p_lift_map<P1>(move(p1));
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

