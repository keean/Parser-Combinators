//============================================================================
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// parser_combinators.hpp

#ifndef PARSER_COMBINATORS_HPP
#define PARSER_COMBINATORS_HPP

#include <istream>
#include <stdexcept>
#include <vector>
#include <tuple>
#include <type_traits>
#include <memory>
#include <cassert>
#include <iterator>
#include <utility>
#include "function_traits.hpp"

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

struct is_alnum {
    using is_predicate_type = true_type;
    string const name = "alphanumeric";
    is_alnum() {}
    bool operator() (int const c) const {
        return ::isalnum(c) != 0;
    }
} const is_alnum;

struct is_alpha {
    using is_predicate_type = true_type;
    string const name = "alphabetic";
    is_alpha() {}
    bool operator() (int const c) const {
        return ::isalpha(c) != 0;
    }
} const is_alpha;

struct is_blank {
    using is_predicate_type = true_type;
    string const name = "blank";
    is_blank() {}
    bool operator() (int const c) const {
        return ::isblank(c) != 0;
    }
} const is_blank;

struct is_cntrl {
    using is_predicate_type = true_type;
    string const name = "control";
    is_cntrl() {}
    bool operator() (int const c) const {
        return ::iscntrl(c) != 0;
    }
} const is_cntrl;

struct is_digit {
    using is_predicate_type = true_type;
    string const name = "digit";
    is_digit() {}
    bool operator() (int const c) const {
        return ::isdigit(c) != 0;
    }
} const is_digit;

struct is_graph {
    using is_predicate_type = true_type;
    string const name = "graph";
    is_graph() {}
    bool operator() (int const c) const {
        return ::isgraph(c) != 0;
    }
} const is_graph;

struct is_lower {
    using is_predicate_type = true_type;
    string const name = "lowercase";
    is_lower() {}
    bool operator() (int const c) const {
        return ::islower(c) != 0;
    }
} const is_lower;

struct is_print {
    using is_predicate_type = true_type;
    string const name = "printable";
    is_print() {}
    bool operator() (int const c) const {
        return ::isprint(c) != 0;
    }
} const is_print;

struct is_punct {
    using is_predicate_type = true_type;
    string const name = "punctuation";
    is_punct() {}
    bool operator() (int const c) const {
        return ::ispunct(c) != 0;
    }
} const is_punct;

struct is_space {
    using is_predicate_type = true_type;
    string const name = "space";
    is_space() {}
    bool operator() (int const c) const {
        return ::isspace(c) != 0;
    }
} const is_space;

struct is_upper {
    using is_predicate_type = true_type;
    string const name = "uppercase";
    is_upper() {}
    bool operator() (int const c) const {
        return ::isupper(c) != 0;
    }
} const is_upper;

struct is_xdigit {
    using is_predicate_type = true_type;
    string const name = "hexdigit";
    is_xdigit() {}
    bool operator() (int const c) const {
        return ::isxdigit(c) != 0;
    }
} const is_xdigit;

//----------------------------------------------------------------------------
// Any single character

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
// Combining character predicates

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
// Parsing Errors

struct parse_error : public runtime_error {

    template <typename Iterator, typename Range>
    static string message(string const& what, string const& pb,
        Iterator const &f, Iterator const &l, Range const &r
    ) {
        stringstream err;

        Iterator i(r.first);
        Iterator line_start(r.first);
        int row = 1;
        while ((i != f) && (i != r.last)) {
            if (*i == '\n') {
                ++row;
                line_start = ++i;
            } else {
                ++i;
            }
        }

        err << what << " at line: " << row
            << " column: " << f - line_start + 1 << endl;

        for (Iterator i(line_start); (*i != '\n') && (i != r.last); ++i) {
            err << static_cast<char>(*i);
        }
        err << endl;

        i = line_start;
        while (i != f) {
            err << ' ';
            ++i;
        }
        err << '^';

        if (i != l) {
            ++i;
            while (i != l) {
                err << '-';
                ++i;
            }
            err << "^";
        }
        
        err << endl << pb << endl;

        return err.str();
    }

    template <typename Iterator, typename Range>
    parse_error(string const& what, string const& pb,
        Iterator const &f, Iterator const &l, Range const &r
    ) : runtime_error(message(what, pb, f, l, r)) {}
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
string format_name(P const& p, int const rank) {
    if (p.rank > rank) {
        return "(" + p.name + ")";
    } else {
        return p.name;
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
    using result_type = string;
    int const rank = 0;
    string const name;

    explicit recogniser_accept(Predicate const& p) : p(p), name(p.name) {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        string *result = nullptr,
        Inherit* st = nullptr
    ) const {
        int sym = *i;
        if (i == r.last) {
            sym = EOF;
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
};

template <typename P, typename = typename P::is_predicate_type>
recogniser_accept<P> const accept(P const &p) {
    return recogniser_accept<P>(p);
}

//-----------------------------------------------------------------------------
// String Parser.

class accept_str {
    string const s;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = string;
    int const rank = 0;
    string const name;

    explicit accept_str(string const& s) : s(s), name("\"" + s + "\"") {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        string *result = nullptr,
        Inherit* st = nullptr
    ) const {
        for (auto j = s.begin(); j != s.end(); ++j) {
            if (*i != *j || i == r.last) {
                return false;
            }
            ++i;
        }
        if (result != nullptr) {
            result->append(s);
        }
        return true;
    }
};

//============================================================================
// Constant Parsers: succ, fail

//----------------------------------------------------------------------------
// Always succeeds.

struct parser_succ {
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = void;
    int const rank = 0;
    string const name = "succ";

    parser_succ() {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        void *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return true;
    }
} const succ;

//----------------------------------------------------------------------------
// Always fails.

struct parser_fail {
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = void;
    int const rank = 0;
    string const name = "fail";

    parser_fail() {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        void *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return false;
    }
} const fail;

//============================================================================
// Lifting String Recognisers to Parsers, and Parsers up one level: any, all

//----------------------------------------------------------------------------
// as soon as one parser succeeds, pass result to user supplied functor

template <typename Functor, typename Inherit, typename = void> class call_any {}; 

template <typename Functor, typename Inherit>
class call_any<Functor, Inherit, typename enable_if<is_same<Inherit, void>::value>::type> {
    Functor const f;

public: 
    explicit call_any(Functor const& f) : f(f) {}
    
    template <typename Result, typename Rs, size_t... I>
    void any(Result* r, int j, Rs& rs, Inherit* st, size_t...) {
        f(r, j, get<I>(rs)...);
    }
};

template <typename Functor, typename Inherit>
class call_any<Functor, Inherit, typename enable_if<!is_same<Inherit, void>::value>::type> {
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
                    throw parse_error(e.what(), name, first, i, r);
                }
            }
            return true;
        }
        return false;
    }

public:
    int const rank = 0;
    string const name;

    explicit fmap_choice(Functor const& f, Parsers const&... ps)
        : f(f), ps(ps...), name("<" + concat(" | ", format_name(ps, rank)...) + ">") {}

    template <typename Iterator, typename Range, typename Inherit,
        typename Is = typename range<0, sizeof...(Parsers)>::type>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return fmap_any(i, r, Is(), result, st);
    }
};

template <typename F, typename... PS>
fmap_choice<F, PS...> const any(F const& f, PS const&... ps) {
    return fmap_choice<F, PS...>(f, ps...);
}

//----------------------------------------------------------------------------
// If all parsers succeed, pass all results as arguments to user supplied functor

template <typename Functor, typename Inherit, typename = void> class call_all {}; 

template <typename Functor, typename Inherit>
class call_all<Functor, Inherit, typename enable_if<is_same<Inherit, void>::value>::type> {
    Functor const f;

public: 
    explicit call_all(Functor const& f) : f(f) {}
    
    template <typename Result, typename Rs, size_t... I>
    void all(Result* r, Rs& rs, Inherit* st, size_t...) {
        f(r, get<I>(rs)...);
    }
};

template <typename Functor, typename Inherit>
class call_all<Functor, Inherit, typename enable_if<!is_same<Inherit, void>::value>::type> {
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
                    throw parse_error(e.what(), name, first, i, r);
                }
            }
            return true;
        }
        return false;
    }

public:
    int const rank = 0;
    string const name;

    explicit fmap_sequence(Functor const& f, Parsers const&... ps)
        : f(f), ps(ps...), name("<" + concat(", ", format_name(ps, rank)...) + ">") {}

    template <typename Iterator, typename Range, typename Inherit = void,
        typename Is = typename range<0, sizeof...(Parsers)>::type>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return fmap_all(i, r, Is(), result, st);
    }
};

template <typename F, typename... PS>
fmap_sequence<F, PS...> const all(F const& f, PS const&... ps) {
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
    using result_type = typename least_general<Parser1, Parser2>::result_type;
    int const rank = 1;
    string const name;

    combinator_choice(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2)
        , name(format_name(p1, rank) + " | " + format_name(p2, rank)) {}

    template <typename Iterator, typename Range, typename Inherit = void>
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
            throw parse_error("failed parser consumed input", p1.name, first, i, r);
        }
        if (p2(i, r, result, st)) {
            return true;
        }
        return false;
    }
};

template <typename P1, typename P2,
    typename = typename enable_if<is_same<typename P1::is_parser_type, true_type>::value
        || is_same<typename P1::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_same<typename P2::is_parser_type, true_type>::value
        || is_same<typename P2::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_compatible<typename P1::result_type, typename P2::result_type>::value>::type>
combinator_choice<P1, P2> const operator|| (P1 const& p1, P2 const& p2) {
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
    using result_type = typename least_general<Parser1, Parser2>::result_type;
    int const rank = 0;
    string const name;

    combinator_sequence(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2)
        , name(format_name(p1, rank) +  ", " + format_name(p2, rank)) {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return p1(i, r, result, st) && p2(i, r, result, st);
    }
};

template <typename P1, typename P2,
    typename = typename enable_if<is_same<typename P1::is_parser_type, true_type>::value
        || is_same<typename P1::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_same<typename P2::is_parser_type, true_type>::value
        || is_same<typename P2::is_handle_type, true_type>::value>::type,
    typename = typename enable_if<is_compatible<typename P1::result_type, typename P2::result_type>::value>::type>
combinator_sequence<P1, P2> const operator&& (P1 const& p1, P2 const& p2) {
    return combinator_sequence<P1, P2>(p1, p2);
}

//----------------------------------------------------------------------------
// Accept the parser zero or more times.

template <typename Parser> class combinator_many {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = typename Parser::result_type;
    int const rank = 0;
    string const name;

    explicit combinator_many(Parser const& p) : p(p), name("{" + p.name + "}") {}

    template <typename Iterator, typename Range, typename Inherit = void>
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
            throw parse_error("failed many-parser consumed input", p.name, first, i, r);
        }
        return true;
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
combinator_many<P> const many(P const& p) {
    return combinator_many<P>(p);
}

//----------------------------------------------------------------------------
// Exception parser

template <typename Parser> class combinator_except {
    Parser const p;
    typename Parser::result_type const x;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = typename Parser::result_type;
    int const rank = 0;
    string const name;

    combinator_except(typename Parser::result_type const& x, Parser const& p)
        : p(p), x(x), name(p.name + " - \"" + x + "\"") {}

    template <typename Iterator, typename Range, typename Inherit = void> 
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        if (p(i, r, result, st)) {
            if (result != nullptr) {
                return x != *result;
            }
            return true;
        }
        return false;
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
combinator_except<P> const except(typename P::result_type const& x, P const& p) {
    return combinator_except<P>(x, p);
}

//============================================================================
// Run-time polymorphism

//----------------------------------------------------------------------------
// Handle: holds a runtime-polymorphic parser. It is mutable so that it can be
// defined before the parser is assigned.

template <typename Iterator, typename Range, typename Synthesize = void, typename Inherit = void>
class parser_handle {

    struct holder_base {
        virtual ~holder_base() {}

        virtual bool parse(
            Iterator& i,
            Range const &r,
            Synthesize* result = nullptr,
            Inherit* st = nullptr
        ) const = 0;
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
    };

    shared_ptr<holder_base const> p;

public:
    using is_parser_type = false_type;
    using is_handle_type = true_type;
    using result_type = Synthesize;
    string name = "<handle>";
    int const rank = 0;

    parser_handle() {}

    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value>::type>
    parser_handle(P const &q) : p(new holder_poly<P>(q)), name(q.name) {} 

    // note: initialise name before moving q.
    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value>::type>
    parser_handle(P &&q) : p(new holder_poly<P>(forward<P>(q))), name(q.name) {}

    parser_handle(parser_handle &&q) : p(move(q.p)), name(q.name) {}

    parser_handle(parser_handle const &q) : p(q.p), name(q.name) {}

    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value>::type>
    parser_handle& operator= (P const &q) {
        name = q.name;
        p = shared_ptr<holder_base const>(new holder_poly<P>(q));
        return *this;
    }

    parser_handle& operator= (parser_handle const &q) {
        name = q.name;
        p = q.p;
        return *this;
    }

    template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type,true_type>::value>::type>
    parser_handle& operator= (P &&q) {
        name = q.name;
        p = shared_ptr<holder_base const>(new holder_poly<P>(forward<P>(q)));
        return *this;
    }

    parser_handle& operator= (parser_handle &&q) {
        name = q.name;
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
};

//----------------------------------------------------------------------------
// Reference: because the handle is mutable it breaks the assumptions of the
// static const parser combinators (which take a copy of the argument parsers).
// As such the combinators would take a copy of the unassigned empty handle, 
// before it gets assigned the parser. The reference provides a const container that
// is copied by the parser combinators, so that when the handle is assigned they
// see the change.

// possibly depricated? Might be better to use handle (above) and reference
// (below) consistently.

template <typename Iterator, typename Range, typename Synthesize = void, typename Inherit = void>
class parser_reference {

    shared_ptr<parser_handle<Iterator, Range, Synthesize, Inherit>> const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = Synthesize;
    int const rank = 0;
    string const name;

    parser_reference() : p(new parser_handle<Iterator, Range, Synthesize, Inherit>()), name(p.name) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_reference(P const &q) : p(new parser_handle<Iterator, Range, Synthesize, Inherit>(q)) {}

    explicit parser_reference(parser_reference const &q) : p(q.p) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_reference(P &&q) : p(new parser_handle<Iterator, Range, Synthesize, Inherit>(forward<P>(q))) {}

    explicit parser_reference(parser_reference &&q) : p(move(q.p)) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_reference const& operator= (P const &q) const {
        *p = q;
        return *this;
    }

    parser_reference const& operator= (parser_reference const &q) const {
        *p = *(q.p);
        return *this;
    }

    template <typename P, typename = typename P::is_parser_type>
    parser_reference const& operator= (P &&q) const {
        *p = forward<P>(q);
        return *this;
    }

    parser_reference const& operator= (parser_reference &&q) const {
        *p = *(q.p);
        return *this;
    }

    bool operator() (
        Iterator &i,
        Range const &r,
        Synthesize* result = nullptr,
        Inherit* st = nullptr
    ) const {
        assert(p != nullptr);
        return (*p)(i, r, result, st);
    }
};

//----------------------------------------------------------------------------
// This is an alternative reference that still works when the self-reference is
// on a single line definition. Ideally this would be combined with the above.
// (this can't be used on r-values).

template <typename Parser>
class parser_ref {
    Parser const& p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = typename Parser::result_type;
    int const rank = 0;
    string const name;

    explicit parser_ref(string const& name, Parser const &q) : p(q), name(name) {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (Iterator &i, Range const &r, result_type *result = nullptr, Inherit* st = nullptr) const {
        return p(i, r, result, st);
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_ref<P> reference(string const& name, P &p) {
    return parser_ref<P>(name, p);
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
    using result_type = void;
    int const rank;
    string const name;

    explicit combinator_discard(Parser const& q)
        : p(q), rank(q.rank), name(q.name) {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        typename Parser::result_type *const discard_result = nullptr;
        return p(i, r, discard_result, st);
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
combinator_discard<P> const discard(P const& p) {
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
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    explicit parser_log(string const& s, Parser const& q)
        : p(q), msg(s), rank(q.rank), name(q.name) {}

    template <typename Iterator, typename Range, typename Inherit = void>
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
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
parser_log<P> log(string const& s, P const& p) {
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
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    explicit parser_try(Parser const& q)
        : p(q), rank(q.rank), name(q.name) {}

    template <typename Iterator, typename Range, typename Inherit = void>
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
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
parser_try<P> attempt(P const& p) {
    return parser_try<P>(p);
}

//----------------------------------------------------------------------------
// Convert fail to error

template <typename Parser>
class parser_strict {
    Parser const p;
    string const err;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    parser_strict(string const& s, Parser const& q)
        : p(q), err(s), rank(q.rank), name(q.name) {}

    template <typename Iterator, typename Range, typename Inherit = void>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        Iterator const first = i;
        if (!p(i, r, result, st)) {
            throw parse_error(err, p.name, first, i, r);
        }
        return true;
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
parser_strict<P> strict(string const& s, P const& p) {
    return parser_strict<P>(s, p);
}

//----------------------------------------------------------------------------
// Name a parser (for better error readability)

template <typename Parser>
class parser_name {
    Parser const p;

public:
    using is_parser_type = true_type;
    using is_handle_type = false_type;
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    parser_name(string const& s, Parser const& q)
        : p(q), rank(q.rank), name(s) {}

    template <typename Iterator, typename Range, typename Inherit>
    bool operator() (
        Iterator &i,
        Range const &r,
        result_type *result = nullptr,
        Inherit* st = nullptr
    ) const {
        return p(i, r, result, st);
    }
};

template <typename P, typename = typename enable_if<is_same<typename P::is_parser_type, true_type>::value
    || is_same<typename P::is_handle_type, true_type>::value>::type>
parser_name<P> name(string const& s, P const& p) {
    return parser_name<P>(s, p);
}

//============================================================================
// Some derived definitions for convenience: option, some

//----------------------------------------------------------------------------
// Optionally accept the parser once.

constexpr char b_opt[] = "[";
constexpr char e_opt[] = "]";

template <typename P> auto option(P const& p)
-> decltype(name(b_opt + p.name + e_opt, p || succ)) {
    return name(b_opt + p.name + e_opt, p || succ);
}

//----------------------------------------------------------------------------
// Accept the parser one or more times.

constexpr char b_some[] = "{";
constexpr char e_some[] = "}-";

template <typename P> auto some(P const& p)
-> decltype(name(b_some + p.name + e_some, p && many(p))) {
    return name(b_some + p.name + e_some, p && many(p));
}

//----------------------------------------------------------------------------
// Accept one parser separated by another

constexpr char b_sep[] = "{";
constexpr char s_sep[] = ", ";
constexpr char e_sep[] = "}";

template <typename P, typename Q> auto sep_by(P const& p, Q const& q)
-> decltype(name(p.name + b_sep + q.name + s_sep + p.name + e_sep,
        p && many(discard(q) && p))) {
    return name(p.name + b_sep + q.name + s_sep + p.name + e_sep,
        p && many(discard(q) && p));
}

//----------------------------------------------------------------------------
// Lazy Tokenisation, by skipping the whitespace after each token we leave 
// the input stream in a position where parsers can fail on the first
// character without requiring backtracking. This requires using the 
// "first_token" parser as the first parser to initialise lazy tokenization
// properly.

// skip to start of first token.
auto first_token = discard(many(accept(is_space)));

template <typename R> auto tokenise (R const& r)
-> decltype(name(r.name, r && first_token)) {
    return name(r.name, r && first_token);
}

#endif // PARSER_COMBINATORS_HPP
