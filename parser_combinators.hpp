//============================================================================
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// parser_combinators.hpp

#include <istream>
#include <stdexcept>
#include <vector>
#include <tuple>
#include <type_traits>
#include <memory>
#include <cassert>
#include <iterator>
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
// File Stream With Location (Row/Col) and Exceptions

struct location {
    int begin;
    int pos;
    int row;
    int col;
    location() : begin(0), pos(0), row(1), col(1) {}
};

class parse_error : public runtime_error {

    static string const make_what(
        string const& w, streambuf *in, string const& n, location const& l, int e
    ) {
        stringstream err;
        string s;

        in->pubseekpos(l.begin);
        int i = l.begin;
        while ((i < e) && (in->sgetc() != EOF)) {
            s.push_back(in->sbumpc());
            ++i;
        }
        while ((in->sgetc() != EOF) && (in->sgetc() != '\n')) {
            s.push_back(in->sbumpc());
            ++i;
        }
        s.push_back('\n');
        i = l.begin;
        while ((i < l.pos) && (in->sgetc() != EOF)) {
            s.push_back(' ');
            ++i;
        }
        if (i++ < e) {
            s.push_back('^');
        }
        while ((i < e) && (in->sgetc() != EOF)) {
            s.push_back('-');
            ++i;
        }
        s.push_back('^');

        err <<  w << ": "
            << "line " << l.row
            << ", column " << l.col << ".\n"
            << s << "\n"
            << n << "\n";

        return err.str();
    }

public:
    streambuf *const in;
    string const name;
    location const loc;
    int const end;

    parse_error(string const& what, streambuf *in, string const& name, location const& s, int e)
        : runtime_error(make_what(what, in, name, s, e)), in(in), name(name), loc(s), end(e) {}
};

class pstream {
    streambuf* in;
    vector<location> stack;
    
    location loc;
    int end;
    int sym;

public:
    pstream(istream &f) : in(f.rdbuf()), end(0), sym(in->sgetc()) {}

    void error(string const& err, string const& name, location const& l) {
        throw parse_error(err, in, name, l, end);
    }

    void next() {
        in->snextc();
        sym = in->sgetc();
        ++loc.pos;
        if (loc.pos > end) {
            end = loc.pos;
        }
        if (sym == '\n') {
            ++loc.row;
            loc.col = 0;
            loc.begin = loc.pos;
        } else if (::isprint(sym)) {
            ++loc.col;
        }
    }

    location const& get_location() {
        return loc;
    }

    int get_sym() {
        return sym;
    }

    int size() {
        return stack.size();
    }
    
    void checkpoint() {
        stack.push_back(loc);
    }

    void backtrack() {
        loc = stack.back();
        in->pubseekpos(loc.pos);
        sym = in->sgetc();
    }

    void commit() {
        stack.pop_back();
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
    using result_type = string;
    int const rank = 0;
    string const name;

    explicit recogniser_accept(Predicate const& p) : p(p), name(p.name) {}

    bool operator() (pstream &in, string *result = nullptr) const {
        int const sym = in.get_sym();
        if (!p(sym)) {
            return false;
        }
        in.next();
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
    using result_type = string;
    int const rank = 0;
    string const name;

    explicit accept_str(string const& s) : s(s), name("\"" + s + "\"") {}

    bool operator() (pstream &in, string *result = nullptr) const {
        for (int i = 0; i < s.size(); ++i) {
            if (s[i] != in.get_sym()) {
                return false;
            }
            in.next();
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
    using result_type = void;
    int const rank = 0;
    string const name = "succ";

    parser_succ() {}

    bool operator() (pstream &in, void *result = nullptr) const {
        return true;
    }
} const succ;

//----------------------------------------------------------------------------
// Always fails.

struct parser_fail {
    using is_parser_type = true_type;
    using result_type = void;
    int const rank = 0;
    string const name = "fail";

    parser_fail() {}

    bool operator() (pstream &in, void *result = nullptr) const {
        return false;
    }
} const fail;

//============================================================================
// Lifting String Recognisers to Parsers, and Parsers up one level: any, all

//----------------------------------------------------------------------------
// as soon as one parser succeeds, pass result to user supplied functor

template <typename Functor, typename... Parsers> class fmap_choice {
    using functor_traits = function_traits<Functor>;
    using tuple_type = tuple<Parsers...>;
    using tmp_type = tuple<typename Parsers::result_type...>;

public:
    using is_parser_type = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

private:
    tuple_type const ps;
    Functor const f;

    template <typename Rs, size_t I0, size_t... Is> 
    int any_parsers(pstream &in, Rs &rs, size_t, size_t...) const {
        if (get<I0>(ps)(in, &get<I0>(rs))) {
            return I0;
        }
        return any_parsers<Rs, Is...>(in, rs, Is...);
    }

    template <typename Rs, size_t I0>
    int any_parsers(pstream &in, Rs &rs, size_t) const {
        if (get<I0>(ps)(in, &get<I0>(rs))) {
            return I0;
        }
        return -1;
    }

    template <size_t... I> 
    bool fmap_any(pstream &in, size_sequence<I...> seq, result_type *result) const {
        tmp_type tmp {};
        location const start = in.get_location();
        int const i = any_parsers<tmp_type, I...>(in, tmp, I...);
        if (i >= 0) {
            if (result != nullptr) {
                try {
                    f(result, i, get<I>(tmp)...);
                } catch (runtime_error &e) {
                    in.error(e.what(), name, start);
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

    template <typename Is = typename range<0, sizeof...(Parsers)>::type>
    bool operator() (pstream &in, result_type *result = nullptr) const {
        return fmap_any(in, Is(), result);
    }
};

template <typename F, typename... PS>
fmap_choice<F, PS...> const any(F const& f, PS const&... ps) {
    return fmap_choice<F, PS...>(f, ps...);
}

//----------------------------------------------------------------------------
// If all parsers succeed, pass all results as arguments to user supplied functor

template <typename Functor, typename... Parsers> class fmap_sequence {
    using functor_traits = function_traits<Functor>;
    using tuple_type = tuple<Parsers...>;
    using tmp_type = tuple<typename Parsers::result_type...>;

public:
    using is_parser_type = true_type;
    using result_type = typename remove_pointer<typename functor_traits::template argument<0>::type>::type;

private:
    tuple_type const ps;
    Functor const f;

    template <typename Rs, size_t I0, size_t... Is> 
    bool all_parsers(pstream &in, Rs &rs, size_t, size_t...) const {
        if (get<I0>(ps)(in, &get<I0>(rs))) {
            return all_parsers<Rs, Is...>(in, rs, Is...);
        }
        return false;
    }

    template <typename Rs, size_t I0>
    bool all_parsers(pstream &in, Rs &rs, size_t) const {
        return get<I0>(ps)(in, &get<I0>(rs));
    }

    template <size_t... I> 
    bool fmap_all(pstream &in, size_sequence<I...> seq, result_type *result) const {
        tmp_type tmp {};
        location const start = in.get_location();
        if (all_parsers<tmp_type, I...>(in, tmp, I...)) {
            if (result != nullptr) {
                try {
                    f(result, get<I>(tmp)...);
                } catch (runtime_error &e) {
                    in.error(e.what(), name, start);
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

    template <typename Is = typename range<0, sizeof...(Parsers)>::type>
    bool operator() (pstream &in, result_type *result = nullptr) const {
        return fmap_all(in, Is(), result);
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
    using result_type = typename least_general<Parser1, Parser2>::result_type;
    int const rank = 1;
    string const name;

    combinator_choice(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2)
        , name(format_name(p1, rank) + " | " + format_name(p2, rank)) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        location const start = in.get_location();
        if (p1(in, result)) {
            return true;
        }
        int const end = in.get_location().pos;
        if (start.pos != end) {
            in.error("failed parser consumed input", p1.name, start);
        }
        if (p2(in, result)) {
            return true;
        }
        return false;
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_parser_type,
    typename = typename P2::is_parser_type,
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
    using result_type = typename least_general<Parser1, Parser2>::result_type;
    int const rank = 0;
    string const name;

    combinator_sequence(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2)
        , name(format_name(p1, rank) +  ", " + format_name(p2, rank)) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        return p1(in, result) && p2(in, result);
    }
};

template <typename P1, typename P2,
    typename = typename P1::is_parser_type,
    typename = typename P2::is_parser_type,
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
    using result_type = typename Parser::result_type;
    int const rank = 0;
    string const name;

    explicit combinator_many(Parser const& p) : p(p), name("{" + p.name + "}") {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        location start = in.get_location();
        while (p(in, result)) {
            start = in.get_location();
        }
        int const end = in.get_location().pos;
        if (start.pos != end) {
            in.error("failed parser consumed input", p.name, start);
        }
        return true;
    }
};

template <typename P, typename = typename P::is_parser_type>
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
    using result_type = typename Parser::result_type;
    int const rank = 0;
    string const name;

    combinator_except(typename Parser::result_type const& x, Parser const& p)
        : p(p), x(x), name(p.name + " - \"" + x + "\"") {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        if (p(in, result)) {
            if (result != nullptr) {
                return x != *result;
            }
            return true;
        }
        return false;
    }
};

template <typename P, typename = typename P::is_parser_type>
combinator_except<P> const except(typename P::result_type const& x, P const& p) {
    return combinator_except<P>(x, p);
}

//============================================================================
// Run-time polymorphism

//----------------------------------------------------------------------------
// Handle: holds a runtime-polymorphic parser. It is mutable so that it can be
// defined before the parser is assigned.

template <typename Result_Type>
class parser_handle {

    struct holder_base {
        virtual ~holder_base() {}
        virtual bool parse(pstream &in, Result_Type *result = nullptr) const = 0;
    }; 

    template <typename Parser>
    class holder_poly : public holder_base {
        Parser const p;

    public:
        explicit holder_poly(Parser const &q) : p(q) {}
        explicit holder_poly(Parser &&q) : p(forward<Parser>(q)) {}

        virtual bool parse(pstream &in, Result_Type *result) const override {
            return p(in, result);
        }
    };

    shared_ptr<holder_base const> p;

public:
    using is_parser_type = true_type;
    using result_type = Result_Type;
    string name = "<handle>";
    int const rank = 0;

    parser_handle() {}

    template <typename P, typename = typename P::is_parser_type>
    parser_handle(P const &q) : p(new holder_poly<P>(q)), name(q.name) {} 

    explicit parser_handle(parser_handle const &q) : p(q.p), name(q.name) {}

    // note: initialise name before moving q.
    template <typename P, typename = typename P::is_parser_type>
    parser_handle(P &&q) : p(new holder_poly<P>(forward<P>(q))), name(q.name) {}

    explicit parser_handle(parser_handle &&q) : p(move(q.p)), name(q.name) {}

    template <typename P, typename = typename P::is_parser_type>
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

    template <typename P, typename = typename P::is_parser_type>
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

    bool operator() (pstream &in, Result_Type *result = nullptr) const {
        assert(p != nullptr);
        return p->parse(in, result);
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

template <typename Result_Type>
class parser_reference {

    shared_ptr<parser_handle<Result_Type>> const p;

public:
    using is_parser_type = true_type;
    using result_type = Result_Type;
    int const rank = 0;
    string const name;

    parser_reference() : p(new parser_handle<Result_Type>()), name(p.name) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_reference(P const &q) : p(new parser_handle<Result_Type>(q)) {}

    explicit parser_reference(parser_reference const &q) : p(q.p) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_reference(P &&q) : p(new parser_handle<Result_Type>(forward<P>(q))) {}

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

    bool operator() (pstream &in, Result_Type *result = nullptr) const {
        assert(p != nullptr);
        return (*p)(in, result);
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
    using result_type = typename Parser::result_type;
    int const rank = 0;
    string const name;

    explicit parser_ref(string const& name, Parser const &q) : p(q), name(name) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        return p(in, result);
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
    using result_type = void;
    int const rank;
    string const name;

    explicit combinator_discard(Parser const& q)
        : p(q), rank(q.rank), name(q.name) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        typename Parser::result_type *const discard_result = nullptr;
        return p(in, discard_result);
    }
};

template <typename P, typename = typename P::is_parser_type>
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
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    explicit parser_log(string const& s, Parser const& q)
        : p(q), msg(s), rank(q.rank), name(q.name) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        location const x = in.get_location();

        bool const b = p(in, result);

        if (b) {
            cout << msg << ": ";

            if (result != nullptr) {
                cout << *result;
            }
            cout << " @" << x.pos << " - " << in.get_location().pos << "(" << in.size() << ")\n";
        }

        return b;
    }
};

template <typename P, typename = typename P::is_parser_type>
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
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    explicit parser_try(Parser const& q)
        : p(q), rank(q.rank), name(q.name) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        in.checkpoint();
        if (p(in, result)) {
            in.commit();
            return true;
        }
        in.backtrack();
        in.commit();
        return false;
    }
};

template <typename P, typename = typename P::is_parser_type>
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
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    parser_strict(string const& s, Parser const& q)
        : p(q), err(s), rank(q.rank), name(q.name) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        location const start = in.get_location();
        if (!p(in, result)) {
            in.error(err, p.name, start);
        }
        return true;
    }
};

template <typename P, typename = typename P::is_parser_type>
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
    using result_type = typename Parser::result_type;
    int const rank;
    string const name;

    parser_name(string const& s, Parser const& q)
        : p(q), rank(q.rank), name(s) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        return p(in, result);
    }
};

template <typename P, typename = typename P::is_parser_type>
parser_name<P> name(string const& s, P const& p) {
    return parser_name<P>(s, p);
}

//============================================================================
// Some derived definitions for convenience: option, some

//----------------------------------------------------------------------------
// Optionally accept the parser once.

template <typename P> auto option(P const& p)
-> decltype(name("[" + p.name + "]", p || succ)) {
    return name("[" + p.name + "]", p || succ);
}

//----------------------------------------------------------------------------
// Accept the parser one or more times.

template <typename P> auto some(P const& p)
-> decltype(name("{" + p.name + "}-", p && many(p))) {
    return name("{" + p.name + "}-", p && many(p));
}

//----------------------------------------------------------------------------
// Accept one parser separated by another

template <typename P, typename Q> auto sep_by(P const& p, Q const& q) 
-> decltype(name(p.name + " {" + q.name + ", " + p.name + "}",
        p && many(discard(q) && p))) {
    return name(p.name + " {" + q.name + ", " + p.name + "}",
        p && many(discard(q) && p));
}

//----------------------------------------------------------------------------
// Lazy Tokenisation, by skipping the whitespace after each token we leave 
// the input stream in a position where parsers can fail on the first
// character without requiring backtracking. This requires using the 
// "next_token" parser as the first parser to initialise lazy tokenization
// properly.

// skip to start of next token.
auto next_token = discard(many(accept(is_space)));

template <typename R> auto tokenise (R const& r)
-> decltype(name(r.name, r && next_token)) {
    return name(r.name, r && next_token);
}

