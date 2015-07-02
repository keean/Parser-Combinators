//============================================================================
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// parser_combinators.hpp

#ifndef PARSER_COMBINATORS_HPP
#define PARSER_COMBINATORS_HPP

#include <istream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <map>
#include <tuple>
#include <type_traits>
#include <memory>
#include <cassert>
#include <iterator>
#include <utility>
#include <type_traits>
#include "function_traits.hpp"

using namespace std;

//============================================================================
// Character Predicates

struct is_any {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_any() {};
    bool operator() (int const c) const {
        return c != EOF;
    }
    string name() const {
        return "anything";
    }
} constexpr is_any;

struct is_alnum {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_alnum() {}
    bool operator() (int const c) const {
        return ::isalnum(c) != 0;
    }
    string name() const {
        return "alphanumeric";
    }
} constexpr is_alnum;

struct is_alpha {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_alpha() {}
    bool operator() (int const c) const {
        return ::isalpha(c) != 0;
    }
    string name() const {
        return "alphabetic";
    }
} constexpr is_alpha;

struct is_blank {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_blank() {}
    bool operator() (int const c) const {
        return ::isblank(c) != 0;
    }
    string name() const {
        return "blank";
    }
} constexpr is_blank;

struct is_cntrl {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_cntrl() {}
    bool operator() (int const c) const {
        return ::iscntrl(c) != 0;
    }
    string name() const {
        return "control";
    }
} constexpr is_cntrl;

struct is_digit {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_digit() {}
    bool operator() (int const c) const {
        return ::isdigit(c) != 0;
    }
    string name() const {
        return "digit";
    }
} constexpr is_digit;

struct is_graph {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_graph() {}
    bool operator() (int const c) const {
        return ::isgraph(c) != 0;
    }
    string name() const {
        return "graphic";
    }
} constexpr is_graph;

struct is_lower {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_lower() {}
    bool operator() (int const c) const {
        return ::islower(c) != 0;
    }
    string name() const {
        return "lowercase";
    }
} constexpr is_lower;

struct is_print {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_print() {}
    bool operator() (int const c) const {
        return ::isprint(c) != 0;
    }
    string name() const {
        return "printable";
    }
} constexpr is_print;

struct is_punct {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_punct() {}
    bool operator() (int const c) const {
        return ::ispunct(c) != 0;
    }
    string name() const {
        return "punctuation";
    }
} constexpr is_punct;

struct is_space {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_space() {}
    bool operator() (int const c) const {
        return ::isspace(c) != 0;
    }
    string name() const {
        return "space";
    }
} constexpr is_space;

struct is_upper {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_upper() {}
    bool operator() (int const c) const {
        return ::isupper(c) != 0;
    }
    string name() const {
        return "uppercase";
    }
} constexpr is_upper;

struct is_xdigit {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_xdigit() {}
    bool operator() (int const c) const {
        return ::isxdigit(c) != 0;
    }
    string name() const {
        return "hexdigit";
    }
} constexpr is_xdigit;

struct is_eol {
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr is_eol() {}
    bool operator() (int const c) const {
        return c == '\n';
    }
    string name() const {
        return "EOL";
    }
} constexpr is_eol;

//----------------------------------------------------------------------------
// Any single character

class is_char {
    int const k;

public:
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr explicit is_char(char const c)
        : k(c) {}
    bool operator() (int const c) const {
        return k == c;
    }
    string name() const {
        return "'" + string(1, k) + "'";
    }
};

is_char constexpr is_eof(EOF);

//----------------------------------------------------------------------------
// Combining character predicates

template <typename P,  typename = typename P::is_predicate_type>
string format_name(P const& p, int const rank) {
    if (p.rank > rank) {
        return "(" + p.name() + ")";
    } else {
        return p.name();
    }
}

template <typename P1, typename P2> class is_either {
    P1 const p1;
    P2 const p2;

public:
    using is_predicate_type = true_type;
    static constexpr int rank = 1;
    constexpr is_either(P1 const& p1, P2 const& p2)
        : p1(p1), p2(p2) {}
    bool operator() (int const c) const {
        return p1(c) || p2(c);
    }
    string name() const {
        return p1.name() + " | " + p2.name();
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_predicate_type,
    typename = typename P2::is_predicate_type>
constexpr is_either<P1, P2> const operator|| (P1 const& p1, P2 const& p2) {
    return is_either<P1, P2>(p1, p2);
}



template <typename P1, typename P2> class is_except {
    P1 const p1;
    P2 const p2;

public:
    using is_predicate_type = true_type;
    static constexpr int rank = 0;
    constexpr explicit is_except(P1 const& p1, P2 const& p2) 
        : p1(p1), p2(p2) {}
    bool operator() (int const c) const {
        return p1(c) && !p2(c);
    }
    string name() const {
        return format_name(p1, rank) + " - " + format_name(p2, rank);
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_predicate_type,
    typename = typename P2::is_predicate_type>
constexpr is_except<P1, P2> const operator- (P1 const& p1, P2 const& p2) {
    return is_except<P1, P2>(p1, p2);
}

//===========================================================================
// Default Inherited Attribute

struct default_inherited {};

//===========================================================================
// Parsing Errors

using unique_defs = map<string, string>;

struct parse_error : public runtime_error {

    template <typename Parser, typename Iterator, typename Range>
    static string message(string const& what, Parser const& p,
        Iterator const &f, Iterator const &l, Range const &r
    ) {
        stringstream err;

        Iterator i(r.first);
        Iterator line_start(r.first);
        int row = 1;
        while ((i != r.last) && (i != f)) {
            if (*i == '\n') {
                ++row;
                line_start = ++i;
            } else {
                ++i;
            }
        }

        err << what << " at line: " << row
            << " column: " << f - line_start + 1 << endl;

        bool in = true;
        for (Iterator i(line_start); (i != r.last) && (in || *i != '\n'); ++i) {
            if (i == l) {
                in = false;
            }
            if (is_space(*i)) {
                err << ' ';
            } else {
                err << static_cast<char>(*i);
            }
        }
        err << endl;

        i = line_start;
        while (i != f) {
            err << ' ';
            ++i;
        }

        err << '^';
        ++i;

        if (i != l) {
            ++i;
            while (i != l) {
                err << '-';
                ++i;
            }
            err << "^";
        }
        
        err << endl << "expecting: ";
       
        unique_defs defs;
        err << p.ebnf(&defs) << endl << "where:" << endl;

        for (auto const& d : defs) {
            err << "\t" << d.first << " = " << d.second << ";" << endl;
        }

        return err.str();
    }

    template <typename Parser, typename Iterator, typename Range>
    parse_error(string const& what, Parser const& p,
        Iterator const &f, Iterator const &l, Range const &r
    ) : runtime_error(message(what, p, f, l, r)) {}
};

//============================================================================
// Type Helpers

//----------------------------------------------------------------------------
// Make tuples of integer sequences from begining to end - 1 of range
template <size_t... Is> struct size_sequence {};
template <size_t Begin, size_t End, size_t... Is> struct range : range<Begin, End - 1, End - 1, Is...> {};
template <size_t Begin, size_t... Is> struct range<Begin, Begin, Is...> : size_sequence<Is...> {};

template <typename F, typename A, typename T, size_t I0, size_t... Is>
A fold_tuple2(F f, A a, T&& t, size_t, size_t...) {
    return fold_tuple2<F, A, T, Is...>(f, f(a, get<I0>(t)), t, Is...);
}

template <typename F, typename A, typename T, size_t I0>
A fold_tuple2(F f, A a, T&& t, size_t) {
    return f(a, get<I0>(t));
}

template <typename F, typename A, typename T, size_t... Is> 
A fold_tuple1(F f, A a, T&& t, size_sequence<Is...>) {
    return fold_tuple2<F, A, T, Is...>(f, a, t, Is...);
}

template <typename F, typename A, typename... Ts>
A fold_tuple(F f, A a, tuple<Ts...> const& t) {
    return fold_tuple1(f, a, t, range<0, sizeof...(Ts)>());
}

//----------------------------------------------------------------------------
// Can a pointer to one type be implicitly converted into a pointer to the other.

template <typename A, typename B> struct is_compat {
    using PA = typename add_pointer<A>::type;
    using PB = typename add_pointer<B>::type;
    static constexpr bool value = is_convertible<PA, PB>::value;
};

template <typename A, typename B> struct is_compatible {
    static constexpr bool value = is_compat<A, B>::value || is_compat<B, A>::value;
};

//----------------------------------------------------------------------------
// Choose the result type of two parsers which a pointer the other result type
// can be conveted into a pointer to.  For example given a parsers with result types
// void and int, the least general is int, because (void*) can be implicitely converted
// to (int*). If there is no possible implicit converstion result_type is undefined.
template <typename P1, typename P2, typename = void> struct least_general {};

template <typename P1, typename P2> struct least_general <P1, P2,
    typename enable_if<is_compat<typename P2::result_type, typename P1::result_type>::value 
    && !is_same<typename P1::result_type, typename P2::result_type>::value>::type> {
    using result_type = typename P2::result_type;
};

template <typename P1, typename P2> struct least_general <P1, P2,
    typename enable_if<is_compat<typename P1::result_type, typename P2::result_type>::value
    || is_same<typename P1::result_type, typename P2::result_type>::value>::type> {
    using result_type = typename P1::result_type;
};

//----------------------------------------------------------------------------
// Concatenate multiple string template arguments into a single string.

string concat(string const& sep, string const& str) {
    return str;
}

template <typename... Strs> string concat(string const& sep, string const& str, Strs const&... strs) {
    string s = str;
    int const unpack[] {0, (s += sep + strs, 0)...};
    return s;
}

//----------------------------------------------------------------------------
// Use EBNF precedence for parser naming.

template <typename P,  typename = typename P::is_parser_type>
string format_name(P const& p, int const rank, unique_defs* defs = nullptr) {
    if (p.rank > rank) {
        return "(" + p.ebnf(defs) + ")";
    } else {
        return p.ebnf(defs);
    }
}

//============================================================================
// Primitive String Recognisers: accept, accept_str

//----------------------------------------------------------------------------
// Stream is advanced if symbol matches, and symbol is appended to result.

template <typename Predicate> class recogniser_accept {
    Predicate const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = false_type;
    using result_type = string;
    int const rank;

    constexpr explicit recogniser_accept(Predicate const& p) : p(p), rank(p.rank) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        string *result = nullptr,
        Inherit* st = nullptr
    ) const {
        int sym;
        if (i == r.last) {
            sym = EOF;
        } else {
            sym = *i;
        }
        if (!p(sym)) {
            return false;
        }
        ++i;
        if (result != nullptr) {
            result->push_back(sym);
        }
        return true;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.name();
    }
};

template <typename P, typename = typename P::is_predicate_type>
constexpr recogniser_accept<P> accept(P const &p) {
    return recogniser_accept<P>(p);
}

//-----------------------------------------------------------------------------
// String Parser.

class accept_str {
    char const* s;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = false_type;
    using result_type = string;
    int const rank = 0;

    constexpr explicit accept_str(char const* s) : s(s) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        string *result = nullptr,
        Inherit* st = nullptr
    ) const {
        for (auto j = s; *j != 0;  ++j) {
            if (i == r.last || *i != *j) {
                return false;
            }
            ++i;
        }
        if (result != nullptr) {
            result->append(s);
        }
        return true;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return "\"" + string(s) + "\"";
    }
};

//============================================================================
// Constant Parsers: succ, fail

//----------------------------------------------------------------------------
// Always succeeds.

struct parser_succ {
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = false_type;
    using result_type = void;
    int const rank = 0;

    constexpr parser_succ() {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        void *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return true;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return "succ";
    }
} constexpr succ;

//----------------------------------------------------------------------------
// Always fails.

struct parser_fail {
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = false_type;
    using result_type = void;
    int const rank = 0;

    constexpr parser_fail() {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        void *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return false;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return "fail";
    }
} constexpr fail;

//============================================================================
// Lifting String Recognisers to Parsers, and Parsers up one level: any, all

//----------------------------------------------------------------------------
// as soon as one parser succeeds, pass result to user supplied functor

template <typename Functor, typename Inherit, typename = void> class call_any {}; 

template <typename Functor, typename Inherit>
class call_any<Functor, Inherit, typename enable_if<is_same<Inherit, default_inherited>::value>::type> {
    Functor const f;

public: 
    explicit call_any(Functor const& f) : f(f) {}
    
    template <typename Result, typename Rs, size_t... I>
    void any(Result* r, int j, Rs& rs, Inherit* st, size_t...) {
        f(r, j, get<I>(rs)...);
    }
};

template <typename Functor, typename Inherit>
class call_any<Functor, Inherit, typename enable_if<!is_same<Inherit, default_inherited>::value>::type> {
    Functor const f;

public: 
    explicit call_any(Functor const& f) : f(f) {}
    
    template <typename Result, typename Rs, size_t... I>
    void any(Result* r, int j, Rs& rs, Inherit* st, size_t...) {
        f(r, j, get<I>(rs)..., st);
    }
};

template <typename Functor, typename... Parsers> class fmap_choice {
    using functor_traits = function_traits<Functor>;
    using tuple_type = tuple<Parsers...>;
    using tmp_type = tuple<typename Parsers::result_type...>;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

private:
    tuple_type const ps;
    Functor const f;

    template <typename Iterator, typename Range, typename Inherit, typename Rs, size_t I0, size_t... Is> 
    int any_parsers(Iterator &i, Range const &r, Inherit* st, Rs &rs, size_t, size_t...) const {
        if (get<I0>(ps)(i, r, &get<I0>(rs), st)) {
            return I0;
        }
        return any_parsers<Iterator, Range, Inherit, Rs, Is...>(i, r, st, rs, Is...);
    }

    template <typename Iterator, typename Range, typename Inherit, typename Rs, size_t I0>
    int any_parsers(Iterator &i, Range const &r, Inherit* st, Rs &rs, size_t) const {
        if (get<I0>(ps)(i, r, &get<I0>(rs), st)) {
            return I0;
        }
        return -1;
    }

    template <typename Iterator, typename Range, typename Inherit, size_t... I> bool fmap_any(
        Iterator &i,
        Range const &r,
        size_sequence<I...> seq,
        result_type *result,
        Inherit* st
    ) const {
        tmp_type tmp {};
        Iterator const first = i;
        int const j = any_parsers<Iterator, Range, Inherit, tmp_type, I...>(i, r, st, tmp, I...);
        if (j >= 0) {
            if (result != nullptr) {
                call_any<Functor, Inherit> call_f(f);
                try {
                    call_f.template any<result_type, tmp_type, I...>(result, j, tmp, st, I...);
                } catch (runtime_error &e) {
                    throw parse_error(e.what(), *this, first, i, r);
                }
            }
            return true;
        }
        return false;
    }

public:
    int const rank = 1;

    constexpr explicit fmap_choice(Functor const& f, Parsers const&... ps)
        : f(f), ps(ps...) {}

    template <typename Iterator, typename Range, typename Inherit>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return fmap_any(i, r, range<0, sizeof...(Parsers)>(), result, st);
    }

    class choice_ebnf {
        int const rank;
        unique_defs* defs;

    public:
        choice_ebnf(int r, unique_defs* d) : rank(r), defs(d) {}
        template <typename P>
        string operator() (string const& s, P&& p) const {
            if (s.length() == 0) {
                return format_name(p, rank, defs);
            }
            return s + " | "  + format_name(p, rank, defs);
        }
    };

    string ebnf(unique_defs* defs = nullptr) const {
        return fold_tuple(choice_ebnf(rank, defs), string(), ps);
    }
};

template <typename F, typename... PS>
constexpr fmap_choice<F, PS...> const any(F const& f, PS const&... ps) {
    return fmap_choice<F, PS...>(f, ps...);
}

//----------------------------------------------------------------------------
// If all parsers succeed, pass all results as arguments to user supplied functor

template <typename Functor, typename Inherit, typename = void> class call_all {}; 

template <typename Functor, typename Inherit>
class call_all<Functor, Inherit, typename enable_if<is_same<Inherit, default_inherited>::value>::type> {
    Functor const f;

public: 
    explicit call_all(Functor const& f) : f(f) {}
    
    template <typename Result, typename Rs, size_t... I>
    void all(Result* r, Rs& rs, Inherit* st, size_t...) {
        f(r, get<I>(rs)...);
    }
};

template <typename Functor, typename Inherit>
class call_all<Functor, Inherit, typename enable_if<!is_same<Inherit, default_inherited>::value>::type> {
    Functor const f;

public: 
    explicit call_all(Functor const& f) : f(f) {}
    
    template <typename Result, typename Rs, size_t... I>
    void all(Result* r, Rs& rs, Inherit* st, size_t...) {
        f(r, get<I>(rs)..., st);
    }
};

template <typename Functor, typename... Parsers> class fmap_sequence {
    using functor_traits = function_traits<Functor>;
    using tuple_type = tuple<Parsers...>;
    using tmp_type = tuple<typename Parsers::result_type...>;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

private:
    tuple_type const ps;
    Functor const f;

    template <typename Iterator, typename Range, typename Inherit, typename Rs, size_t I0, size_t... Is> 
    bool all_parsers(Iterator &i, Range const &r, Inherit* st, Rs &rs, size_t, size_t...) const {
        if (get<I0>(ps)(i, r, &get<I0>(rs), st)) {
            return all_parsers<Iterator, Range, Inherit, Rs, Is...>(i, r, st, rs, Is...);
        }
        return false;
    }

    template <typename Iterator, typename Range, typename Inherit, typename Rs, size_t I0>
    bool all_parsers(Iterator &i, Range const &r, Inherit* st, Rs &rs, size_t) const {
        return get<I0>(ps)(i, r, &get<I0>(rs), st);
    }

    template <typename Iterator, typename Range, typename Inherit, size_t... I>
    bool fmap_all(Iterator &i, Range const &r, size_sequence<I...> seq, result_type *result, Inherit* st) const {
        tmp_type tmp {};
        Iterator const first = i;
        if (all_parsers<Iterator, Range, Inherit, tmp_type, I...>(i, r, st, tmp, I...)) {
            if (result != nullptr) {
                call_all<Functor, Inherit> call_f(f);
                try {
                    call_f.template all<result_type, tmp_type, I...>(result, tmp, st, I...);
                } catch (runtime_error &e) {
                    throw parse_error(e.what(), *this, first, i, r);
                }
            }
            return true;
        }
        return false;
    }

public:
    int const rank = 0;

    constexpr fmap_sequence(Functor const& f, Parsers const&... ps)
        : f(f), ps(ps...) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return fmap_all(i, r, range<0, sizeof...(Parsers)>(), result, st);
    }

    class sequence_ebnf {
        int const rank;
        unique_defs* defs;

    public:
        sequence_ebnf(int r, unique_defs* d) : rank(r), defs(d) {}
        template <typename P>
        string operator() (string const &s, P&& p) const {
            if (s.size() == 0) {
                return format_name(p, rank, defs);
            }
            return s + ", "  + format_name(p, rank, defs);
        }
    };

    string ebnf(unique_defs* defs = nullptr) const {
        if (tuple_size<tuple_type>::value == 1) {
            return format_name(get<0>(ps), rank, defs);
        } else {
            return fold_tuple(sequence_ebnf(rank, defs), string(), ps);
        }
    }
};

template <typename F, typename... PS>
constexpr fmap_sequence<F, PS...> const all(F const& f, PS const&... ps) {
    return fmap_sequence<F, PS...>(f, ps...);
}

//============================================================================
// Combinators For Both Parsers and Recognisers: ||, &&, many 

//----------------------------------------------------------------------------
// Run the second parser only if the first fails.

template <typename Parser1, typename Parser2> class combinator_choice { 
    Parser1 const p1;
    Parser2 const p2;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects =
        integral_constant<bool, Parser1::has_side_effects::value || Parser2::has_side_effects::value>;
    using result_type = typename least_general<Parser1, Parser2>::result_type;
    int const rank = 1;

    constexpr combinator_choice(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator const first = i;
        if (p1(i, r, result, st)) {
            return true;
        }
        if (first != i) {
            throw parse_error("failed parser consumed input", p1, first, i, r);
        }
        if (p2(i, r, result, st)) {
            return true;
        }
        return false;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return format_name(p1, rank, defs) + " | " + format_name(p2, rank, defs);
    }
};

template <typename P1, typename P2,
    typename = typename enable_if<is_same<typename P1::is_parser_type, true_type>::value
        || is_same<typename P1::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_same<typename P2::is_parser_type, true_type>::value
        || is_same<typename P2::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_compatible<typename P1::result_type, typename P2::result_type>::value, pair<typename P1::result_type, typename P2::result_type>>::type>
constexpr combinator_choice<P1, P2> const operator|| (P1 const& p1, P2 const& p2) {
    return combinator_choice<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------
// Run the second parser only if the first succeeds. 

template <typename Parser1, typename Parser2> class combinator_sequence {
    Parser1 const p1;
    Parser2 const p2;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects =
        integral_constant<bool, Parser1::has_side_effects::value || Parser2::has_side_effects::value>;
    using result_type = typename least_general<Parser1, Parser2>::result_type;
    int const rank = 0;

    constexpr combinator_sequence(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return p1(i, r, result, st) && p2(i, r, result, st);
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return format_name(p1, rank, defs) + ", " + format_name(p2, rank, defs);
    }
};

template <typename P1, typename P2,
    typename = typename enable_if<is_same<typename P1::is_parser_type, true_type>::value
        || is_same<typename P1::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_same<typename P2::is_parser_type, true_type>::value
        || is_same<typename P2::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_compatible<typename P1::result_type, typename P2::result_type>::value>::type>
constexpr combinator_sequence<P1, P2> operator&& (P1 const& p1, P2 const& p2) {
    return combinator_sequence<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------
// Accept the parser zero or more times.

template <typename Parser> class combinator_many {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank = 0;

    constexpr explicit combinator_many(Parser const& p) : p(p) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator first = i;
        while (p(i, r, result, st)) {
            first = i;
        }
        if (first != i) {
            throw parse_error("failed many-parser consumed input", p, first, i, r);
        }
        return true;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return "{" + p.ebnf(defs) + "}";
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
constexpr combinator_many<P> const many(P const& p) {
    return combinator_many<P>(p);
}

//----------------------------------------------------------------------------
// Exception parser

template <typename Parser> class combinator_except {
    Parser const p;
    char const* x;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank = 0;

    constexpr combinator_except(char const* x, Parser const& p) : p(p), x(x) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited> 
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        result_type tmp;
        if (p(i, r, &tmp, st)) {
            if (x != tmp) {
                if (result != nullptr) {
                    *result = tmp;
                }
                return true;
            }
        }
        return false;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.ebnf(defs) + " - \"" + x + "\"";
    }
};

template <typename P, typename = typename enable_if<
    is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value
    >::type>
constexpr combinator_except<P> const operator- (P const& p, char const* x) {
    return combinator_except<P>(x, p);
}

//============================================================================
// Run-time polymorphism

//----------------------------------------------------------------------------
// Handle: holds a runtime-polymorphic parser. It is mutable so that it can be
// defined before the parser is assigned.

template <typename Iterator, typename Range, typename Synthesize = void, typename Inherit = default_inherited>
class parser_handle {

    struct holder_base {
        virtual ~holder_base() {}

        virtual bool parse(
            Iterator& i,
            Range const &r,
            Synthesize* result = nullptr,
            Inherit* st = nullptr
        ) const = 0;

        virtual string ebnf(unique_defs* defs = nullptr) const = 0;
    }; 

    template <typename Parser>
    class holder_poly : public holder_base {
        Parser const p;

    public:
        explicit holder_poly(Parser const &q) : p(q) {}
        explicit holder_poly(Parser &&q) : p(forward<Parser>(q)) {}

        virtual bool parse(
            Iterator &i,
            Range const &r,
            Synthesize* result = nullptr,
            Inherit* st = nullptr
        ) const override {
            return p(i, r, result, st);
        }

        virtual string ebnf(unique_defs* defs = nullptr) const override {
            return p.ebnf(defs);
        }
    };

    shared_ptr<holder_base const> p;

public:
    using is_parser_type = false_type;
    using is_handle_type = true_type;
    using has_side_effects = true_type; // have to assume it does.
    using result_type = Synthesize;
    int const rank = 0;

    constexpr parser_handle() {}

    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value>::type>
    constexpr parser_handle(P const &q) : p(new holder_poly<P>(q)) {} 

    // note: initialise name before moving q.
    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value>::type>
    constexpr parser_handle(P &&q) : p(new holder_poly<P>(forward<P>(q))) {}

    constexpr parser_handle(parser_handle&& q) : p(move(q.p)) {}

    constexpr parser_handle(parser_handle const &q) : p(q.p) {}

    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value>::type>
    parser_handle& operator= (P const &q) {
        p = shared_ptr<holder_base const>(new holder_poly<P>(q));
        return *this;
    }

    parser_handle& operator= (parser_handle const &q) {
        p = q.p;
        return *this;
    }

    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type,true_type>::value>::type>
    parser_handle& operator= (P &&q) {
        p = shared_ptr<holder_base const>(new holder_poly<P>(forward<P>(q)));
        return *this;
    }

    parser_handle& operator= (parser_handle &&q) {
        p = move(q.p);
        return *this;
    }

    bool operator() (
        Iterator &i,
        Range const &r,
        Synthesize* result = nullptr,
        Inherit* st = nullptr
    ) const {
        assert(p != nullptr);
        return p->parse(i, r, result, st);
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p->ebnf(defs);
    }
};

//----------------------------------------------------------------------------
// Reference Parser, used to create a self reference in a recursive parser.

template <typename Parser>
class parser_ref {
    Parser const* p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank = 0;
    char const* name;

    constexpr parser_ref(char const* name, Parser const* q) : p(q), name(name) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (Iterator &i, Range const &r, result_type *result = nullptr, Inherit* st = nullptr) const {
        return (*p)(i, r, result, st);
    }

    string ebnf(unique_defs* defs = nullptr) const {
        if (defs != nullptr) {
            auto i = defs->find(name);
            if (i == defs->end()) {
                auto i = (defs->emplace(name, name)).first;
                string const n = p->ebnf(defs);
                i->second = n;
            }
        }
        return name;
    }
};

template <typename P, typename = typename P::is_parser_type>
constexpr parser_ref<P> reference(char const* name, P const* p) {
    return parser_ref<P>(name, p);
}

//----------------------------------------------------------------------------
// Fixed Point Parser, neater way to define simple recursive parsers.

template <typename F> class parser_fix {
    using parser_type = typename function_traits<F>::return_type;
    parser_type const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename parser_type::has_side_effects;
    using result_type = typename parser_type::result_type;
    int const rank = 0;
    char const* name;

    constexpr explicit parser_fix(char const* n, F f) : p(f(reference(n, &p))) , name(n) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (Iterator &i, Range const &r, result_type *result = nullptr, Inherit* st = nullptr) const {
        return p(i, r, result, st);
    }

    string ebnf(unique_defs* defs = nullptr) const {
        string const n = p.ebnf(defs);
        if (defs != nullptr) {
            defs->emplace(name, n);
        }
        return name;
    }
};

template <typename F> // , typename = typename function_traits<F>::result_type::is_parser_type>
constexpr parser_fix<F> fix(char const* n, F f) {
    return parser_fix<F>{n, f};
}

//============================================================================
// Parser modifiers: are not visible in parser naming.

//----------------------------------------------------------------------------
// Discard the result of the parser (and the result type), keep succeed or fail.

template <typename Parser> class combinator_discard {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = void;
    int const rank;

    constexpr explicit combinator_discard(Parser const& q)
        : p(q), rank(q.rank) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        typename Parser::result_type *const discard_result = nullptr;
        return p(i, r, discard_result, st);
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.ebnf(defs);
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
constexpr combinator_discard<P> const discard(P const& p) {
    return combinator_discard<P>(p);
}

//----------------------------------------------------------------------------
// Logging Parser

template <typename Parser> 
class parser_log {
    Parser const p;
    string const msg;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank;

    constexpr explicit parser_log(string const& s, Parser const& q)
        : p(q), msg(s), rank(q.rank) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator const x = i;

        bool const b = p(i, r, result, st);

#ifdef DEBUG
        if (b) {
            cout << msg << ": ";

            if (result != nullptr) {
                cout << *result;
            }

            cout << " @" << (x - r.first) << " - " << (i - r.first) << "\n";
        }
#endif

        return b;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.ebnf(defs);
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
constexpr parser_log<P> log(string const& s, P const& p) {
    return parser_log<P>(s, p);
}

//----------------------------------------------------------------------------
// Backtracking Parser

template <typename Parser>
class parser_try {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank;

    constexpr explicit parser_try(Parser const& q) : p(q), rank(q.rank) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator const first = i;
        if (p(i, r, result, st)) {
            return true;
        } 
        i = first;
        return false;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.ebnf(defs);
    }
};

template <typename Parser>
class parser_try_side {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank;

    constexpr explicit parser_try_side(Parser const& q) : p(q), rank(q.rank) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator const first = i;
        Inherit inh;
        if (st != nullptr) {
            inh = *st;
        }
        if (p(i, r, result, st)) {
            return true;
        } 
        i = first;
        if (st != nullptr) {
            *st = inh;
        }
        return false;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.ebnf(defs);
    }
};

template <typename P, typename = typename enable_if<
    (
        is_same<typename P::is_parser_type, true_type>::value
        || is_same<typename P::is_handle_type, true_type>::value
    ) && is_same<typename P::has_side_effects, false_type>::value
>::type>
constexpr parser_try<P> attempt(P const& p) {
    return parser_try<P>(p);
}

template <typename P, typename = typename enable_if<
    (
        is_same<typename P::is_parser_type, true_type>::value
        || is_same<typename P::is_handle_type, true_type>::value
    ) && is_same<typename P::has_side_effects, true_type>::value
>::type>
constexpr parser_try_side<P> attempt(P const& p) {
    return parser_try_side<P>(p);
}

//----------------------------------------------------------------------------
// Convert fail to error

template <typename Parser>
class parser_strict {
    Parser const p;
    char const* err;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank;

    constexpr parser_strict(char const* s, Parser const& q) : p(q), err(s), rank(q.rank) {}

    template <typename Iterator, typename Range, typename Inherit = default_inherited>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator const first = i;
        if (!p(i, r, result, st)) {
            throw parse_error(err, p, first, i, r);
        }
        return true;
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return p.ebnf(defs);
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
constexpr parser_strict<P> strict(char const* s, P const& p) {
    return parser_strict<P>(s, p);
}

//----------------------------------------------------------------------------
// Name a parser (for better error readability)

template <typename Parser, typename Name>
class parser_name {
    Parser const p;
    Name const n;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank;

    constexpr parser_name(Name&& m, int r, Parser const& q)
        : p(q), n(forward<Name>(m)), rank(r) {}

    template <typename Iterator, typename Range, typename Inherit>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return p(i, r, result, st);
    }

    string ebnf(unique_defs* defs = nullptr) const {
        return n(defs);
    }
};

template <typename P, typename N, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
constexpr parser_name<P, N> rename(N&& n, int r, P const& p) {
    return parser_name<P, N>(forward<N>(n), r, p);
}

//----------------------------------------------------------------------------
// Define a sub-parser (for better error readability)

template <typename Parser>
class parser_def {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using has_side_effects = typename Parser::has_side_effects;
    using result_type = typename Parser::result_type;
    int const rank;
    char const* name;

    constexpr parser_def(char const* n, Parser const& q)
        : p(q), rank(q.rank), name(n) {}

    template <typename Iterator, typename Range, typename Inherit>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return p(i, r, result, st);
    }
    
    string ebnf(unique_defs* defs = nullptr) const {
        string const n = p.ebnf(defs);
        if (defs != nullptr) {
            defs->emplace(name, n);
        }
        return name;
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
constexpr parser_def<P> define(char const* s, P const& p) {
    return parser_def<P>(s, p);
}

//============================================================================
// Some derived definitions for convenience: option, some

//----------------------------------------------------------------------------
// Optionally accept the parser once.

template <typename Parser> struct option_name {
    Parser const p;
    constexpr option_name(Parser const& q) : p(q) {}
    string operator() (unique_defs* defs = nullptr) const {
        return "[" + p.ebnf(defs) + "]";
    }
};

template <typename P> constexpr auto option(P const& p)
-> decltype(rename(option_name<P>(p), 0, p || succ)) {
    return rename(option_name<P>(p), 0, p || succ);
}

//----------------------------------------------------------------------------
// Accept the parser one or more times.

template <typename Parser> struct some_name {
    Parser const p;
    constexpr some_name(Parser const& q) : p(q) {}
    string operator() (unique_defs* defs = nullptr) const {
        return "{" + p.ebnf(defs) + "}-";
    }
};

template <typename P> constexpr auto some(P const& p)
-> decltype(rename(some_name<P>(p), 0, p && many(p))) {
    return rename(some_name<P>(p), 0, p && many(p));
}

//----------------------------------------------------------------------------
// Accept one parser separated by another

template <typename P, typename Q> struct sepby_name {
    P const p;
    Q const q;
    constexpr sepby_name(P const& p, Q const& q) : p(p), q(q) {}
    string operator() (unique_defs* defs = nullptr) const {
        return p.ebnf(defs) + ", {" + q.ebnf(defs) + ", " + p.ebnf() + "}";
    }
};

template <typename P, typename Q> constexpr auto sep_by(P const& p, Q const& q)
-> decltype (rename(sepby_name<P, Q>(p, q), 0, p && many(discard(q) && p))) {
    return rename(sepby_name<P, Q>(p, q), 0, p && many(discard(q) && p));
}

//----------------------------------------------------------------------------
// Lazy Tokenisation, by skipping the whitespace after each token we leave 
// the input stream in a position where parsers can fail on the first
// character without requiring backtracking. This requires using the 
// "first_token" parser as the first parser to initialise lazy tokenization
// properly.

// skip to start of first token.
constexpr auto first_token = discard(many(accept(is_space)));

template <typename Parser> struct tok_name {
    Parser const p;
    constexpr tok_name(Parser const& q) : p(q) {}
    string operator() (unique_defs* defs = nullptr) const {
        return p.ebnf(defs);
    }
};

template <typename R> constexpr auto tokenise(R const& r)
-> decltype(rename(tok_name<R>(r), 0, r && first_token)) {
    return rename(tok_name<R>(r), 0, r && first_token);
}

#endif // PARSER_COMBINATORS_HPP
