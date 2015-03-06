#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <sstream>

#include "stream_iterator.hpp"
#include "templateio.hpp"
#include "profile.hpp"

using namespace std;

//============================================================================
// Logic Language Parser.

//----------------------------------------------------------------------------
// Syntactic Structure.

using name_t = set<string>::const_iterator;

struct type_expression {
    virtual ostream& operator<< (ostream& out) const = 0;
};

struct type_variable : public type_expression {
    name_t const name;

    type_variable(name_t n) : name(n) {}

    virtual ostream& operator<< (ostream& out) const override {
        out << *name;
        return out;
    }
};

struct type_struct : public type_expression {
    name_t const name;
    vector<type_expression*> const args;

    template <typename As>
    type_struct(name_t n, As&& as) : name(n), args(forward<As>(as)) {}

    ostream& operator<< (ostream& out) const override {
        out << *name << "(";
        for (auto i = args.cbegin(); i != args.cend(); ++i) {
            (*i)->operator<<(out);
            auto j = i;
            if (++j != args.end()) {
                out << ", ";
            }
        }
        out << ")";
        return out;
    }
};

ostream& operator<< (ostream& out, type_expression* exp) {
    exp->operator<<(out);
    return out;
}

struct clause {
    type_struct* const head;
    vector<type_struct*> const impl;
    set<type_variable*> const reps;

    template <typename Is, typename Rs>
    clause(type_struct* h, Is&& is, Rs&& rs) : head(h), impl(forward<Is>(is)),
        reps(forward<Rs>(rs)) {}
};

ostream& operator<< (ostream& out, clause* cls) {
    cls->head->operator<<(out);
    auto f = cls->impl.cbegin();
    auto const l = cls->impl.cend();
    if (f != l) {
        out << " :-" << endl;
        for (auto i = f; i != l; ++i) {
            out << "\t";
            (*i)->operator<<(out);
            auto j = i;
            if (++j != l) {
                out << "," << endl;
            }
        }
    }
    out << ".";

    auto g = cls->reps.cbegin();
    auto const m = cls->reps.cend();
    if (g != m) {
        out << " [";
        for (auto i = g; i != m; ++i) {
            (*i)->operator<<(out);
            auto j = i;
            if (++j != m) {
                out << ", ";
            }
        }
        out << "]";
    }
    out << endl;

    return out;
}

//----------------------------------------------------------------------------
// Parser State

using var_t = map<string const*, type_variable*>::const_iterator;

struct parser_state {
    set<string> names; // global name table
    map<string const*, type_variable*> variables;
    set<type_variable*> repeated;
    set<type_variable*> repeated_in_goal;

    parser_state() {};
    parser_state(parser_state const&) = delete;
    parser_state& operator= (parser_state const&) = delete;

    name_t get_name(string const& name) {
        name_t const n = names.find(name);
        if (n == names.end()) {
            return names.insert(name).first;
        } else {
            return n;
        }
    }
};

//----------------------------------------------------------------------------
// Grammar

class return_variable {
public:
    return_variable() {}

    void operator() (type_variable** res, string const& name, parser_state* st) const {
        name_t const n = st->get_name(name);
        var_t i = st->variables.find(&(*n));
        if (i == st->variables.end()) {
            type_variable* const var = new type_variable(n);
            st->variables.insert(make_pair(&(*n), var));
            *res = var;
        } else {
            st->repeated.insert(i->second);
            *res = i->second;
        }
    }
} const return_variable;

struct return_args {
    return_args() {}
    void operator() (vector<type_expression*>* res, int n, type_variable* var, type_struct* str, parser_state*) const {
        switch(n) {
            case 0:
                res->push_back(var);
                break;
            case 1:
                res->push_back(str);
                break;
        }
    }
} const return_args;

struct return_struct {
    return_struct() {}
    void operator() (type_struct** res, string const& name, vector<type_expression*>& args, parser_state* st) const {
        name_t n = st->get_name(name);
        *res = new type_struct(n, args);
    }
} const return_struct;

struct return_goal {
    return_goal() {}
    void operator() (type_struct** res, type_struct* str, parser_state* st) const {
        *res = str;
        st->repeated_in_goal = st->repeated;
    }
} const return_goal;

struct return_impl {
    return_impl() {}
    void operator() (vector<type_struct*>* res, type_struct* impl, parser_state*) const {
        res->push_back(impl);
    }
} const return_impl;

struct return_clause {
    return_clause() {}
    void operator() (vector<clause*>* res, type_struct* head, vector<type_struct*>& impl, parser_state* st) const {
        res->push_back(new clause(head, impl, st->repeated_in_goal));
        st->variables.clear();
        st->repeated.clear();
        st->repeated_in_goal.clear();
    }
} const return_clause;

struct return_clause2 {
    return_clause2() {}
    void operator() (vector<clause*>* res, vector<type_struct*>& impl, parser_state* st) const {
        vector<type_expression*> vars;
        for (auto v : st->variables) {
            vars.push_back(v.second);
        }
        res->push_back(new clause(
            new type_struct(st->get_name("goal"), vars), impl, set<type_variable*> {}
        ));

        st->variables.clear();
        st->repeated.clear();
        st->repeated_in_goal.clear();
    }
} const return_clause2;

//----------------------------------------------------------------------------
// Parser

using expression_handle = pstream_handle<string, parser_state>;

auto const atom_tok = tokenise(some(accept(is_lower)) && many(accept(is_alnum || is_char('_'))));
auto const var_tok = tokenise(some(accept(is_upper || is_char('_'))) && many(accept(is_alnum)));
auto const open_tok = tokenise(accept(is_char('(')));
auto const close_tok = tokenise(accept(is_char(')')));
auto const sep_tok = tokenise(accept(is_char(',')));
auto const impl_tok = tokenise(accept_str(":-"));
auto const end_tok = tokenise(accept(is_char('.')));
auto const comment_tok = tokenise(accept(is_char('#')) && many(accept(is_print)) && accept(is_char('\n')));

pstream_handle<type_struct*, parser_state> const structure(pstream_handle<type_struct*, parser_state> const s) {
    return all(return_struct, atom_tok,
        option(discard(open_tok)
        && sep_by(any(return_args, all(return_variable, var_tok), s), discard(sep_tok))
        && discard(close_tok)));
}

pstream_handle<type_struct*, parser_state> const goal = structure(reference("structure", goal));

auto const clause = all(return_clause, all(return_goal, goal),
    option(discard(impl_tok) && sep_by(all(return_impl, goal), discard(sep_tok))) && discard(end_tok))
    || discard(impl_tok) && all(return_clause2, sep_by(all(return_impl, goal), discard(sep_tok)) && discard(end_tok))
    || discard(comment_tok);

struct expression_parser;

template <typename Range>
int parse(Range const &r) {
    auto const parser = first_token && some(clause);
    decltype(parser)::result_type a {}; 
    typename Range::iterator i = r.first;
    parser_state st;

    profile<expression_parser> p;
    if (parser(i, r, &a, &st)) {
        cout << "OK" << endl;
    } else {
        cout << "FAIL" << endl;
    }

    cout << a << endl;
    
    return i - r.first;
}

//----------------------------------------------------------------------------

int main(int const argc, char const *argv[]) {
    if (argc < 1) {
        cerr << "no input files" << endl;
    } else {
        for (int i = 1; i < argc; ++i) {
            profile<expression_parser>::reset();
            stream_range in(argv[i]);
            cout << argv[i] << endl;
            int const chars_read = parse(in);
            double const mb_per_s = static_cast<double>(chars_read) / static_cast<double>(profile<expression_parser>::report());
            cout << "parsed: " << mb_per_s << "MB/s" << endl;
        }
    }
}
