//----------------------------------------------------------------------------
// copyright 2012, 2013, 2014 Keean Schupke
// compile with g++ -std=gnu++11 
// parser.h

#include <iostream>
#include <fstream>
#include <stdexcept>

using namespace std;

//----------------------------------------------------------------------------
// Character Predicates

struct is_space {
    string const name = "space";
    is_space() {}
    bool operator() (int const c) const {
        return ::isspace(c) != 0;
    }
} const is_space;

struct is_digit {
    string const name = "digit";
    is_digit() {}
    bool operator() (int const c) const {
        return ::isdigit(c) != 0;
    }
} const is_digit;

struct is_upper {
    string const name = "uppercase";
    is_upper() {}
    bool operator() (int const c) const {
        return ::isupper(c) != 0;
    }
} const is_upper;

struct is_lower {
    string const name = "lowercase";
    is_lower() {}
    bool operator() (int const c) const {
        return ::islower(c) != 0;
    }
} const is_lower;

struct is_alpha {
    string const name = "alphabetic";
    is_alpha() {}
    bool operator() (int const c) const {
        return ::isalpha(c) != 0;
    }
} const is_alpha;

struct is_alnum {
    string const name = "alphanumeric";
    is_alnum() {}
    bool operator() (int const c) const {
        return ::isalnum(c) != 0;
    }
} const is_alnum;

struct is_print {
    string const name = "printable";
    is_print() {}
    bool operator() (int const c) const {
        return ::isprint(c) != 0;
    }
} const is_print;

class is_char {
    int const k;

public:
    string const name;
    explicit is_char(char const c)
        : k(c), name("'" + string(1, c) + "'") {}
    bool operator() (int const c) const {
        return k == c;
    }
};

//----------------------------------------------------------------------------

template <typename P1, typename P2> class is_either_tmp {
    P1 const p1;
    P2 const p2;

public:
    string const name;
    is_either_tmp(P1&& p1, P2&& p2)
        : p1(forward<P1>(p1)), p2(forward<P2>(p2)), name("(" + p1.name + " or " + p2.name + ")") {}
    bool operator() (int const c) const {
        return p1(c) || p2(c);
    }
};

template <typename P1, typename P2> class is_either_crf {
    P1 const &p1;
    P2 const &p2;

public:
    string const name;
    is_either_crf(P1 const& p1, P2 const& p2)
        : p1(p1), p2(p2), name("(" + p1.name + " or " + p2.name + ")") {}
    bool operator() (int const c) const {
        return p1(c) || p2(c);
    }
};

template <typename P1, typename P2> is_either_tmp<P1, P2> const operator+ (P1&& p1, P2&& p2) {
    return is_either_tmp<P1, P2>(forward<P1>(p1), forward<P2>(p2));
}

template <typename P1, typename P2> is_either_crf<P1, P2> const operator+ (P1 const& p1, P2 const& p2) {
    return is_either_crf<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------

template <typename P1> class is_not_tmp {
    P1 const p1;

public:
    string const name;
    explicit is_not_tmp(P1&& p1) 
        : p1(forward<P1>(p1)), name("~" + p1.name) {}
    bool operator() (int const c) const {
        return !p1(c);
    }
};

template <typename P1> class is_not_crf {
    P1 const &p1;

public:
    string const name;
    explicit is_not_crf(P1 const& p1) 
        : p1(p1), name("~" + p1.name) {}
    bool operator() (int const c) const {
        return !p1(c);
    }
};

template <typename P1> is_not_tmp<P1> const operator~ (P1&& p1) {
    return is_not_tmp<P1>(forward<P1>(p1));
}

template <typename P1> is_not_crf<P1> const operator~ (P1 const& p1) {
    return is_not_crf<P1>(p1);
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

//----------------------------------------------------------------------------
    
template <typename P> class p_accept_tmp {
    P const pred;

public:
    typedef string value_type;

    explicit p_accept_tmp(P&& p) : pred(forward<P>(p)) {}

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

template <typename P> class p_accept_crf {
    P const &pred;

public:
    typedef string value_type;

    explicit p_accept_crf(P const& p) : pred(p) {}

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

template <typename P> const p_accept_tmp<P> accept(P&& p) {
    return p_accept_tmp<P>(forward<P>(p));
}

template <typename P> const p_accept_crf<P> accept(P const& p) {
    return p_accept_crf<P>(p);
}

//----------------------------------------------------------------------------

template <typename P> class p_expect_tmp {
    P const pred;

public:
    typedef string value_type;

    explicit p_expect_tmp(P&& p) : pred(forward<P>(p)) {}

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

template <typename P> class p_expect_crf {
    P const &pred;

public:
    typedef string value_type;

    explicit p_expect_crf(P const& p) : pred(p) {}

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

template <typename P> const p_expect_tmp<P> expect(P&& p) {
    return p_expect_tmp<P>(forward<P>(p));
}

template <typename P> const p_expect_crf<P> expect(P const& p) {
    return p_expect_crf<P>(p);
}

//----------------------------------------------------------------------------

template <typename P1> class p_option_tmp {
    P1 const p1;

public:
    typedef typename P1::value_type value_type;

    explicit p_option_tmp(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        bool const t = p1(p, v);
        return true;
    }
};

template <typename P1> class p_option_crf {
    P1 const &p1;

public:
    typedef typename P1::value_type value_type;

    explicit p_option_crf(P1 const& p1) : p1(p1) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        bool const t = p1(p, v);
        return true;
    }
};

template <typename P1> const p_option_tmp<P1> option(P1&& p1) {
    return p_option_tmp<P1>(forward<P1>(p1));
}

template <typename P1> const p_option_crf<P1> option(P1 const& p1) {
    return p_option_crf<P1>(p1);
}

//----------------------------------------------------------------------------

template <typename P1> class p_many0_tmp {
    P1 const p1;

public:
    typedef typename P1::value_type value_type;

    explicit p_many0_tmp(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        while(p1(p, v));
        return true;
    }
}; 

template <typename P1> class p_many0_crf {
    P1 const &p1;

public:
    typedef typename P1::value_type value_type;

    explicit p_many0_crf(P1 const& p1) : p1(p1) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        while(p1(p, v));
        return true;
    }
}; 

template <typename P1> const p_many0_tmp<P1> many0(P1&& p1) {
    return p_many0_tmp<P1>(forward<P1>(p1));
}

template <typename P1> const p_many0_crf<P1> many0(P1 const& p1) {
    return p_many0_crf<P1>(p1);
}

//----------------------------------------------------------------------------

template <typename P1> class p_many1_tmp {
    P1 const p1;

public:
    typedef typename P1::value_type value_type;

    explicit p_many1_tmp(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (p1(p, v)) {
            while (p1(p, v));
            return true;
        }
        return false;
    }
};

template <typename P1> class p_many1_crf {
    P1 const &p1;

public:
    typedef typename P1::value_type value_type;

    explicit p_many1_crf(P1 const& p1) : p1(p1) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (p1(p, v)) {
            while (p1(p, v));
            return true;
        }
        return false;
    }
};

template <typename P1> const p_many1_tmp<P1> many1(P1&& p1) {
    return p_many1_tmp<P1>(forward<P1>(p1));
}

template <typename P1> const p_many1_crf<P1> many1(P1 const& p1) {
    return p_many1_crf<P1>(p1);
}

//----------------------------------------------------------------------------

template <typename P1, typename P2> class p_either_tmp { 
    P1 const p1;
    P2 const p2;

public:
    typedef typename P1::value_type value_type;

    p_either_tmp(P1&& p1, P2&& p2) : p1(forward<P1>(p1)), p2(forward<P2>(p2)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        return p1(p, v) || p2(p, v);
    }
};

template <typename P1, typename P2> class p_either_crf { 
    P1 const &p1;
    P2 const &p2;

public:
    typedef typename P1::value_type value_type;

    p_either_crf(P1 const& p1, P2 const& p2) : p1(p1), p2(p2) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        return p1(p, v) || p2(p, v);
    }
};

template <typename P1, typename P2> const p_either_tmp<P1, P2> operator|| (P1&& p1, P2&& p2) {
    return p_either_tmp<P1, P2>(forward<P1>(p1), forward<P2>(p2));
}

template <typename P1, typename P2> const p_either_crf<P1, P2> operator|| (P1 const& p1, P2 const& p2) {
    return p_either_crf<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------

template <typename P1, typename P2> class p_sequence_tmp {
    P1 const p1;
    P2 const p2;

public:
    typedef typename P1::value_type value_type;

    p_sequence_tmp(P1&& p1, P2&& p2) : p1(forward<P1>(p1)), p2(forward<P2>(p2)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        return p1(p, v) && p2(p, v);
    }
};

template <typename P1, typename P2> class p_sequence_crf {
    P1 const &p1;
    P2 const &p2;

public:
    typedef typename P1::value_type value_type;

    p_sequence_crf(P1 const& p1, P2 const& p2) : p1(p1), p2(p2) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        return p1(p, v) && p2(p, v);
    }
};

template <typename P1, typename P2> const p_sequence_tmp<P1, P2> operator&& (P1&& p1, P2&& p2) {
    return p_sequence_tmp<P1, P2>(forward<P1>(p1), forward<P2>(p2));
}

template <typename P1, typename P2> const p_sequence_crf<P1, P2> operator&& (P1 const& p1, P2 const& p2) {
    return p_sequence_crf<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------

template <typename P1> class p_lift_vector_tmp {
    P1 const p1;

public:
    typedef vector<typename P1::value_type> value_type;

    p_lift_vector_tmp(P1&& p1) : p1(forward<P1>(p1)) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (v != nullptr) {
            typename P1::value_type v1;
            if (p1(p, &v1)) {
                v->push_back(move(v1));
                return true;
            }
            return false;
        } else {
            return p1(p, nullptr);
        }
    }
};

template <typename P1> class p_lift_vector_crf {
    P1 const &p1;

public:
    typedef vector<typename P1::value_type> value_type;

    p_lift_vector_crf(P1 const& p1) : p1(p1) {}

    bool operator() (fparse &p, value_type *v = nullptr) const {
        if (v != nullptr) {
            typename P1::value_type v1;
            if (p1(p, &v1)) {
                v->push_back(move(v1));
                return true;
            }
            return false;
        } else {
            return p1(p, nullptr);
        }
    }
};

template <typename P1> const p_lift_vector_tmp<P1> lift_vector(P1&& p1) {
    return p_lift_vector_tmp<P1>(forward<P1>(p1));
}

template <typename P1> const p_lift_vector_crf<P1> lift_vector(P1 const& p1) {
    return p_lift_vector_crf<P1>(p1);
}

//----------------------------------------------------------------------------

auto const is_minus = is_char('-');
auto const p_int = option(accept(is_minus)) && many1(accept(is_digit));

struct parse_int {
    typedef int value_type;
    bool operator() (fparse &p, value_type *i = nullptr) const {
        string s;
        if (p_int(p, &s)) {
            if (i != nullptr) {
                *i = stoi(s);
            }
            return true;
        }
        return false;
    }
};

auto const space = many1(accept(is_space));
auto const number = many1(accept(is_digit));
auto const signed_number = option(accept(is_minus)) && number;
auto const name = accept(is_alpha) && many0(accept(is_alnum));

