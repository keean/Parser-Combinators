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

struct parse_error : public runtime_error {
    int const row;
    int const col;
    int const sym;
    string const exp;
    parse_error(string const& what, int row, int col, string const& exp, int sym)
        : runtime_error(what), row(row), col(col), sym(sym), exp(exp) {}
};

class pstream {
    streambuf* in;
    vector<streampos> stack;
    
    int count;
    int row;
    int col;
    int sym;

public:
    pstream(istream &f) : in(f.rdbuf()), count(0), row(1), col(1), sym(in->sgetc()) {}

    void error(string const& err, string const& exp) {
        throw parse_error(err, row, col, exp, sym);
    }

    void next() {
        in->snextc();
        sym = in->sgetc();
        ++count;
        if (sym == '\n') {
            ++row;
            col = 0;
        } else if (::isprint(sym)) {
            ++col;
        }
    }

    int get_count() {
        return count;
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

    void checkpoint() {
        stack.push_back(count);
    }

    void backtrack() {
        count = stack.back();
        in->pubseekpos(count);
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

//============================================================================
// Primitive String Recognisers: accept, expect

//----------------------------------------------------------------------------
// Stream is advanced if symbol matches, and symbol is appended to result.

template <typename Predicate> class recogniser_accept {
    Predicate const p;

public:
    using is_parser_type = true_type;
    using result_type = string;

    explicit recogniser_accept(Predicate const& p) : p(p) {}

    bool operator() (pstream &in, string *result = nullptr) const {
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
recogniser_accept<P> const accept(P const &p) {
    return recogniser_accept<P>(p);
}

//----------------------------------------------------------------------------
// Throw exception if symbol does not match, accept otherwise.

template <typename Predicate> class recogniser_expect {
    Predicate const p;

public:
    using is_parser_type = true_type;
    using result_type = string;

    explicit recogniser_expect(Predicate const& p) : p(p) {}

    bool operator() (pstream &in, string *result = nullptr) const {
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
recogniser_expect<P> const expect(P const& p) {
    return recogniser_expect<P>(p);
}

//============================================================================
// Constant Parsers: succ, fail

//----------------------------------------------------------------------------
// Always succeeds.

struct parser_succ {
    using is_parser_type = true_type;
    using result_type = void;

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
        int const i = any_parsers<tmp_type, I...>(in, tmp, I...);
        if (i >= 0) {
            if (result != nullptr) {
                f(result, i, get<I>(tmp)...);
            }
            return true;
        }
        return false;
    }

public:
    explicit fmap_choice(Functor const& f, Parsers const&... ps) : f(f), ps(ps...) {}

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
        if (all_parsers<tmp_type, I...>(in, tmp, I...)) {
            if (result != nullptr) {
                f(result, get<I>(tmp)...);
            }
            return true;
        }
        return false;
    }

public:
    explicit fmap_sequence(Functor const& f, Parsers const&... ps) : f(f), ps(ps...) {}

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
// Combinators For Both Parsers and Recognisers: ||, &&, many, discard 

//----------------------------------------------------------------------------
// Run the second parser only if the first fails.

template <typename Parser1, typename Parser2> class combinator_choice { 
    Parser1 const p1;
    Parser2 const p2;

public:
    using is_parser_type = true_type;
    using result_type = typename least_general<Parser1, Parser2>::result_type;

    combinator_choice(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        in.checkpoint();
        if (p1(in, result)) {
            in.commit();
            return true;
        }
        in.backtrack();
        in.commit(); // last-call optimisation.
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

    combinator_sequence(Parser1 const& p1, Parser2 const& p2) : p1(p1), p2(p2) {}

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

    explicit combinator_many(Parser const& p) : p(p) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        while (p(in, result));
        return true;
    }
};

template <typename P, typename = typename P::is_parser_type>
combinator_many<P> const many(P const& p) {
    return combinator_many<P>(p);
}

//----------------------------------------------------------------------------
// Discard the result of the parser (and the result type), keep succeed or fail.

template <typename Parser> class combinator_discard {
    Parser const p;

public:
    using is_parser_type = true_type;
    using result_type = void;

    explicit combinator_discard(Parser const& p) : p(p) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        typename Parser::result_type *const discard_result = nullptr;
        return p(in, discard_result);
    }
};

template <typename P, typename = typename P::is_parser_type>
combinator_discard<P> const discard(P const& p) {
    return combinator_discard<P>(p);
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
        explicit holder_poly(Parser const &p) : p(p) {}
        explicit holder_poly(Parser &&q) : p(forward<Parser>(q)) {}

        virtual bool parse(pstream &in, Result_Type *result) const override {
            return p(in, result);
        }
    };

    shared_ptr<holder_base const> p;

public:
    using is_parser_type = true_type;
    using result_type = Result_Type;

    parser_handle() {}

    template <typename P, typename = typename P::is_parser_type>
    parser_handle(P const &q) : p(new holder_poly<P>(q)) {} 

    explicit parser_handle(parser_handle const &q) : p(q.p) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_handle(P &&q) : p(new holder_poly<P>(forward<P>(q))) {}

    explicit parser_handle(parser_handle &&q) : p(move(q.p)) {}

    template <typename P, typename = typename P::is_parser_type>
    parser_handle& operator= (P const &q) {
        p = shared_ptr<holder_base const>(new holder_poly<P>(q));
        return *this;
    }

    parser_handle& operator= (parser_handle const &q) {
        p = q.p;
        return *this;
    }

    template <typename P, typename = typename P::is_parser_type>
    parser_handle& operator= (P &&q) {
        p = shared_ptr<holder_base const>(new holder_poly<P>(forward<P>(q)));
        return *this;
    }

    parser_handle& operator= (parser_handle &&q) {
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

    parser_reference() : p(new parser_handle<Result_Type>()) {}

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

    explicit parser_ref(Parser &q) : p(q) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        return p(in, result);
    }
};

template <typename P> parser_ref<P> reference(P &p) {
    return parser_ref<P>(p);
}

//============================================================================
// Logging Parser

template <typename Parser> 
class parser_log {
    Parser const p;
    string const msg;

public:
    using is_parser_type = true_type;
    using result_type = typename Parser::result_type;

    explicit parser_log(string const& s, Parser const& q) : p(q), msg(s) {}

    bool operator() (pstream &in, result_type *result = nullptr) const {
        bool const b = p(in, result);
        cout << msg << ": ";

        if (b) {
            cout << "succ(";
        } else {
            cout << "fail(";
        }

        if (result != nullptr) {
            cout << *result;
        }

        cout << ") @" << in.get_count() << "\n";
        return b;
    }
};

template <typename P> parser_log<P> log(string const& s, P &p) {
    return parser_log<P>(s, p);
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

//----------------------------------------------------------------------------
// Lazy Tokenisation.

template <typename R> auto tokenise (R const& r) -> decltype(discard(many(accept(is_space))) && r) {
    return discard(many(accept(is_space))) && r;
}

