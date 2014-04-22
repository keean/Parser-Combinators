//----------------------------------------------------------------------------
// copyright 2012, 2013, 2014 Keean Schupke
// compile with g++ -std=gnu++11 
// parser.h

#include <fstream>
#include <stdexcept>

using namespace std;

//----------------------------------------------------------------------------
// Character Predicates

struct is_space {
    string const name = "space";
    bool operator() (int const c) const {
        return ::isspace(c) != 0;
    }
} is_space;

struct is_digit {
    string const name = "digit";
    bool operator() (int const c) const {
        return ::isdigit(c) != 0;
    }
} is_digit;

struct is_upper {
    string const name = "uppercase";
    bool operator() (int const c) const {
        return ::isupper(c) != 0;
    }
} is_upper;

struct is_lower {
    string const name = "lowercase";
    bool operator() (int const c) const {
        return ::islower(c) != 0;
    }
} is_lower;

struct is_alpha {
    string const name = "alphabetic";
    bool operator() (int const c) const {
        return ::isalpha(c) != 0;
    }
} is_alpha;

struct is_alnum {
    string const name = "alphanumeric";
    bool operator() (int const c) const {
        return ::isalnum(c) != 0;
    }
} is_alnum;

struct is_print {
    string const name = "printable";
    bool operator() (int const c) const {
        return ::isprint(c) != 0;
    }
} is_print;

class is_char {
    string const name;
    int const k;

public:
    explicit is_char(char const c)
        : k(c), name("'" + string(1, c) + "'") {}
    bool operator() (int const c) const {
        return k == c;
    }
};

template <typename L, typename R> class is_either {
    string const name;
    L const &a;
    R const &b;

public:
    is_either(L const &a, R const& b)
        : a(a), b(b), name("(" + a.name + " or " + b.name + ")") {}
    bool operator() (int const c) const {
        return a(c) || b(c);
    }
};

template <typename L, typename R> is_either<L, R> const operator+ (L const &l, R const &r) {
    return is_either<L, R>(l, r);
}

template <typename C> class is_not {
    string const name;
    C const &a;

public:
    explicit is_not(C const &a) 
        : a(a), name("~" + a.name) {}
    bool operator() (int const c) const {
        return !a(c);
    }
};

template <typename C> is_not<C> const operator~ (C const &c) {
    return is_not<C>(c);
}

//----------------------------------------------------------------------------
// Recursive Descent Parser

struct parse_error : public runtime_error {
    int const row;
    int const col;
    int const sym;
    string const &exp;
    parse_error(string const &what, int row, int col, string const &exp, int sym)
        : runtime_error(what), row(row), col(col), exp(exp), sym(sym) {}
};

class fparse {
    fstream &in;
    int row;
    int col;
    int sym;

public:
    fparse(fstream &f) : in(f), row(1), col(1), sym(f.get()) {}

    void error(string const& err, string const exp) {
        throw parse_error(err, row, col, exp, sym);
    }

    void next() {
        sym = in.get();
        if (sym == '\n') {
            ++row;
            col = 1;
        } else {
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
    
template <typename C> class p_accept {
    C const &pred;

public:
    explicit p_accept(C const &p) : pred(p) {}

    bool operator() (fparse &p, string *s = nullptr) const {
        int const sym = p.get_sym();
        if (!pred(sym)) {
            return false;
        }
        if (s != nullptr) {
            s->push_back(sym);
        }
        p.next();
        return true;
    }
};

template <typename C> const p_accept<C> accept(C const &p) {
    return p_accept<C>(p);
}
    
template <typename C> class p_expect {
    C const &pred;

public:
    explicit p_expect(C const &p) : pred(p) {}

    bool operator() (fparse &p, string *s = nullptr) const {
        int const sym = p.get_sym();
        if (!pred(sym)) {
            p.error("expected", pred.name);   
        }
        if (s != nullptr) {
            s->push_back(sym);
        }
        p.next();
        return true;
    }
};

template <typename C> const p_expect<C> expect(C const &p) {
    return p_expect<C>(p);
}

template <typename L, typename R> class p_or { 
    L const &l;
    R const &r;

public:
    p_or(L const &l, R const &r) : l(l), r(r) {}

    bool operator() (fparse &p, string *s = nullptr) const {
        return l(p, s) || r(p, s);
    }
};

template <typename L, typename R> const p_or<L, R> operator|| (L const &l, R const &r) {
    return p_or<L, R>(l, r);
}

template <typename L, typename R> class p_and {
    L const &l;
    R const &r;

public:
    p_and(L const &l, R const &r) : l(l), r(r) {}

    bool operator() (fparse &p, string *s = nullptr) const {
        return l(p, s) && r(p, s);
    }
};

template <typename L, typename R> const p_and<L, R> operator&& (L const &l, R const &r) {
    return p_and<L, R>(l, r);
}

template <typename P> class p_many0 {
    P const &p;

public:
    explicit p_many0(P const &p) : p(p) {}

    bool operator() (fparse &f, string *s = nullptr) const {
        while(p(f, s));
        return true;
    }
}; 

template <typename P> const p_many0<P> many0(P const &p) {
    return p_many0<P>(p);
}

template <typename P> class p_many1 {
    P const &p;

public:
    explicit p_many1(P const &p) : p(p) {}

    bool operator() (fparse &s, string *r = nullptr) const {
        if (p(s, r)) {
            while (p(s, r));
            return true;
        }
        return false;
    }
};

template <typename P> const p_many1<P> many1(P const &p) {
    return p_many1<P>(p);
}

template <typename P> class p_option {
    P const &p;

public:
    explicit p_option(P const &p) : p(p) {}

    bool operator() (fparse &s, string *r = nullptr) const {
        bool const t = p(s, r);
        return true;
    }
};

template <typename P> const p_option<P> option(P const &p) {
    return p_option<P>(p);
}

template <typename P, typename V> class p_map {
    P const &p;

public:
    explicit p_map(P const &p) : p(p) {}

    bool operator() (fparse &s, V &v) {
        string r;
        if (p(s, &r)) {
            stringstream t(r);
            t >> v;
            return true;
        }
        return false;
    }
};

template <typename P> const p_map<P> map(P const &p) {
    return p_map<P>(p);
};

auto const is_minus = is_char('-');
auto const space = many1(accept(is_space));
auto const number = many1(accept(is_digit));
auto const signed_number = option(accept(is_minus)) && number;
auto const name = accept(is_alpha) && many0(accept(is_alnum));

