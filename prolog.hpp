#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <map>

#include "stream_iterator.hpp"
#include "templateio.hpp"
#include "profile.hpp"

using namespace std;

//============================================================================
// Logic Language Parser.
//
// A simple parser for a Prolog like language.

//----------------------------------------------------------------------------
// Syntactic Structure.
//
// A global set of names keeps string comparisons for atoms and varibles to
// a single iterator (pointer) comparison. A single type is used for atoms
// and structs, which is combined with a variable type in an expression 
// supertype. Clauses combine heads and goals, and keep track of repeated 
// variables in the head for efficient post-unification cycle checking.

using name_t = std::set<std::string>::const_iterator;

struct name_less {
    bool operator() (name_t const& x, name_t const& y) const {
        return &(*x) < &(*y);
    }
};

struct type_expression {
    virtual void accept(class expr_visitor* v) = 0;
};

struct type_variable : public type_expression {
    name_t const name;
    type_variable(name_t n) : name(n) {}
    virtual void accept(class expr_visitor* v) override;
};

struct type_struct : public type_expression {
    name_t const name;
    vector<type_expression*> const args;
    template <typename As> type_struct(name_t n, As&& as) : name(n), args(forward<As>(as)) {}
    virtual void accept(class expr_visitor* v) override;
};

struct expr_visitor {
    virtual void visit(type_variable *t) = 0;
    virtual void visit(type_struct *t) = 0;
};

void type_variable::accept(expr_visitor* v) {v->visit(this);}
void type_struct::accept(expr_visitor* v) {v->visit(this);}



struct clause {
    type_struct* const head;
    vector<type_struct*> const impl;
    set<type_variable*> const reps;

    template <typename Is, typename Rs>
    clause(type_struct* h, Is&& is, Rs&& rs) : head(h), impl(forward<Is>(is)),
        reps(forward<Rs>(rs)) {}
};



using db_t = multimap<name_t, struct clause*, name_less>; 

struct program {
    set<string> names;
    db_t db;
};

//----------------------------------------------------------------------------
// Show Abstract Syntax

class expr_show : public expr_visitor {
    ostream& out;

    virtual void visit(type_variable* t) override {
        out << *(t->name);
    }

    virtual void visit(type_struct* t) override {
        if (::ispunct((*(t->name))[0]) && t->args.size() == 2) {
            t->args[0]->accept(this);
            cout << " " << *(t->name) << " ";
            t->args[1]->accept(this);
        } else {
            out << *(t->name);
            if (t->args.size() > 0) {
                out << "(";
                for (auto i = t->args.cbegin(); i != t->args.cend(); ++i) {
                    (*i)->accept(this);
                    auto j = i;
                    if (++j != t->args.end()) {
                        out << ", ";
                    }
                }
                out << ")";
            }
        }
    }

public:
    expr_show(ostream& out) : out(out) {}

    void operator() (type_expression* t) {
        t->accept(this);
    }
};

ostream& operator<< (ostream& out, type_expression* exp) {
    expr_show eshow(out);
    eshow(exp);
    return out;
}

ostream& operator<< (ostream& out, clause* cls) {
    expr_show eshow(out);

    eshow(cls->head);
    auto f = cls->impl.cbegin();
    auto const l = cls->impl.cend();
    if (f != l) {
        out << " :-" << endl;
        for (auto i = f; i != l; ++i) {
            out << "\t";
            eshow(*i);
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
            eshow(*i);
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

ostream& operator<< (ostream& out, program const& p) {
    int i = 1, tab = to_string(p.db.size()).size();
    for (auto j = p.db.cbegin(); j != p.db.cend(); ++i, ++j) {
        string pad(" ", tab - to_string(i).size());
        out << pad << i << ". " << j->second;
    }
    return out;
};

//----------------------------------------------------------------------------
// Parser State
//
// Making this uncopyable prevents backtracking being used, as the 'attempt'
// combinator will try and make a copy of the inherited attribute for 
// backtracking. This can be seen as a constraint on the parser (to ensure
// performance) or a safety measure to make sure we deal with copying if we 
// want backtracking.

using var_t = map<name_t, type_variable*>::const_iterator;

struct inherited_attributes {
    set<string>& names; // global name table
    map<name_t, type_variable*, name_less> variables; 
    set<type_variable*> repeated;
    set<type_variable*> repeated_in_goal;

    //inherited_attributes() {};
    inherited_attributes(inherited_attributes const&) = delete;
    inherited_attributes& operator= (inherited_attributes const&) = delete;

    name_t get_name(string const& name) {
        name_t const n = names.find(name);
        if (n == names.end()) {
            return names.insert(name).first;
        } else {
            return n;
        }
    }

    inherited_attributes(set<string>& ns) : names(ns) {}
};

//----------------------------------------------------------------------------
// Grammar
//
// Function objects get passed the results of the sub-parsers, along with the 
// inherited attribute. As the parser-combinators are templated, the inherited
// attribute is whatever type is passed as the final argument to the parser.

struct return_variable {
    return_variable() {}
    void operator() (type_variable** res, string const& name, inherited_attributes* st) const {
        name_t const n = st->get_name(name);
        var_t i = st->variables.find(n);
        if (i == st->variables.end()) {
            type_variable* const var = new type_variable(n);
            st->variables.insert(make_pair(n, var));
            *res = var;
        } else {
            st->repeated.insert(i->second);
            *res = i->second;
        }
    }
} const return_variable;

struct return_args {
    return_args() {}
    void operator() (vector<type_expression*>* res, type_expression* t1, inherited_attributes*) const {
        res->push_back(t1);
    }
} const return_args;

struct return_struct {
    return_struct() {}
    void operator() (type_struct** res, string const& name, vector<type_expression*>& args, inherited_attributes* st) const {
        *res = new type_struct(st->get_name(name), args);
    }
} const return_struct;

struct return_term {
    return_term() {}
    void operator() (type_expression** res, int n, type_variable* v, type_struct *s, inherited_attributes* st) const {
        switch (n) {
            case 0:
                *res = v;
                break;
            case 1:
                *res = s;
                break;
        }
    }
} const return_term;

struct return_op_exp_exp {
    return_op_exp_exp() {}
    void operator() (type_expression** res, type_expression* t1, pair<string, type_expression*> const& t2, inherited_attributes* st) const {
        if (!t2.first.empty()) {
            name_t o = st->get_name(t2.first);
            vector<type_expression*> a {t1, t2.second};
            *res = new type_struct(o, a);
        } else {
            *res = t1;
        }
    }
} const return_op_exp_exp;

struct return_op_var_exp {
    return_op_var_exp() {}
    void operator() (type_struct** res, type_variable* t1, string const& oper, type_expression* t2, inherited_attributes* st) const {
        name_t o = st->get_name(oper);
        vector<type_expression*> a {t1, t2};
        *res = new type_struct(o, a);
    }
} const return_op_var_exp;

struct return_oper_term {
    return_oper_term() {}
    void operator() (
        pair<string, type_expression*>* res,
        string const& oper,
        type_expression* term,
        inherited_attributes* st
    ) const {
        *res = make_pair(oper, term);
    }
} const return_oper_term;    

struct return_op_stc_exp {
    return_op_stc_exp() {}
    void operator() (type_struct** res, type_struct* t1, pair<string, type_expression*> const& t2, inherited_attributes* st) const {
        if (!t2.first.empty()) {
            name_t o = st->get_name(t2.first);
            vector<type_expression*> a {t1, t2.second};
            *res = new type_struct(o, a);
        } else {
            *res = t1;
        }
    }
} const return_op_stc_exp;
 
struct return_head {
    return_head() {}
    void operator() (type_struct** res, type_struct* str, inherited_attributes* st) const {
        *res = str;
        st->repeated_in_goal = st->repeated;
    }
} const return_head;

struct return_goal {
    return_goal() {}
    void operator() (vector<type_struct*>* res, type_struct* impl, inherited_attributes*) const {
        res->push_back(impl);
    }
} const return_goal;

struct return_clause {
    return_clause() {}
    void operator() (db_t* res, type_struct* head, vector<type_struct*>& impl, inherited_attributes* st) const {
        res->emplace(head->name, new clause(head, impl, st->repeated_in_goal));
        st->variables.clear();
        st->repeated.clear();
        st->repeated_in_goal.clear();
    }
} const return_clause;

struct return_goals {
    return_goals() {}
    void operator() (db_t* res, vector<type_struct*>& impl, inherited_attributes* st) const {
        vector<type_expression*> vars;
        for (auto v : st->variables) {
            vars.push_back(v.second);
        }

        name_t n = st->get_name("goal");

        res->emplace(make_pair(n, new clause(
            new type_struct(n, vars), impl, set<type_variable*> {}
        )));

        st->variables.clear();
        st->repeated.clear();
        st->repeated_in_goal.clear();
    }
} const return_goals;

//----------------------------------------------------------------------------
// Parser
//
// The parsers and user grammar are all stateless, so can be const objects and 
// still thread safe. The final composed parser is a regular procedure, all state 
// is either in the state object passed in, or in the returned values.

auto const atom_tok = tokenise(accept(is_lower) && many(accept(is_alnum || is_char('_'))));
auto const var_tok = tokenise(accept(is_upper || is_char('_')) && many(accept(is_alnum || is_char('_'))));
auto const open_tok = tokenise(accept(is_char('(')));
auto const close_tok = tokenise(accept(is_char(')')));
auto const sep_tok = tokenise(accept(is_char(',')));
auto const end_tok = tokenise(accept(is_char('.')));
auto const impl_tok = tokenise(accept_str(":-"));
auto const oper_tok = tokenise(some(accept(
        is_punct - (is_char('_')  || is_char('(')  || is_char(')')  || is_char(','))
    )) - "." - ":-");
auto const comment_tok = tokenise(accept(is_char('#')) && many(accept(is_print)) && accept(is_eol));

// the "definitions" help clean up the EBNF output in error reports
auto const variable = define("variable", all(return_variable, var_tok));
auto const atom = define("atom", atom_tok);
auto const oper = define("operator", oper_tok);

template <typename T> using pshand = pstream_handle<T, inherited_attributes>;

// higher order parsers.

// parse atom and list of arguments.
pshand<type_struct*> recursive_struct(pshand<type_expression*> const t) {
    return define("struct", all(return_struct,
        atom,
        option(discard(open_tok) && sep_by(all(return_args, t), sep_tok) && discard(close_tok))
    ));
}

// parse a term that is either a variable or an atom/struct
pshand<type_expression*> recursive_term(pshand<type_expression*> const t) {
    return define("term", any(return_term,
        variable,
        recursive_struct(t))
    );
}

// parse a term, followed by optional operator and term.
pshand<type_expression*> recursive_oper(pshand<type_expression*> const t) {
    return all(return_op_exp_exp,
        recursive_term(t),
        option(all(return_oper_term, attempt(oper), t))
    );
}

auto const op = fix("op-list", recursive_oper);

auto const structure = define("op-struct", all(return_op_var_exp, variable, oper, op)
    || all(return_op_stc_exp, recursive_struct(op), option(all(return_oper_term, attempt(oper), op))));

auto const comment = define("comment", discard(comment_tok));
auto const goals = define("goals", discard(impl_tok)
    && sep_by(all(return_goal, structure), discard(sep_tok)));
auto const query = define("query", all(return_goals, goals) && discard(end_tok));
auto const clause = define("clause", all(return_clause, all(return_head, structure), option(goals) && discard(end_tok)));

auto const parser = first_token && strict("unexpected character", some(clause || query || comment));

