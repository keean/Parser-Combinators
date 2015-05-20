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

using atom_t = std::set<std::string>::const_iterator;

struct atom_less {
    bool operator() (atom_t const& x, atom_t const& y) const {
        return &(*x) < &(*y);
    }
};

struct managed {
    virtual ~managed() {}
};

struct term : public managed {
    virtual void accept(class term_visitor* v) = 0;
    virtual ~term() {}
};

struct variable : public term {
    atom_t const atom;
    virtual void accept(class term_visitor* v) override;
private:
    friend class program;
    variable(atom_t n) : atom(n) {}
};

struct compound : public term {
    atom_t const atom;
    vector<term*> const args;
    virtual void accept(class term_visitor* v) override;
private:
    friend class program;
    template <typename As> compound(atom_t n, As&& as) : atom(n), args(forward<As>(as)) {}
};

struct term_visitor {
    virtual void visit(variable *t) = 0;
    virtual void visit(compound *t) = 0;
};

void variable::accept(term_visitor* v) {v->visit(this);}
void compound::accept(term_visitor* v) {v->visit(this);}

struct clause : public managed {
    compound* const head;
    vector<compound*> const impl;
    set<variable*> const reps;

private:
    friend class program;
    template <typename Is, typename Rs>
    clause(compound* h, Is&& is, Rs&& rs) : head(h), impl(forward<Is>(is)),
        reps(forward<Rs>(rs)) {}
};

class program {
    vector<unique_ptr<managed>> region;

public:
    set<string> atoms;
    multimap<atom_t, struct clause*, atom_less> db;
    vector<clause*> goals;

    variable* new_variable(atom_t n) {
        variable* v = new variable(n);
        region.emplace_back(v);
        return v;
    }

    template <typename As> compound* new_compound(atom_t n, As&& as) {
        compound* c = new compound(n, forward<As>(as));
        region.emplace_back(c);
        return c;
    }

    template <typename Is, typename Rs>
    clause* new_clause(compound* h, Is&& is, Rs&& rs) {
        clause* c = new clause(h, forward<Is>(is), forward<Rs>(rs));
        region.emplace_back(c);
        return c;
    }
};

//----------------------------------------------------------------------------
// Show Abstract Syntax

class expr_show : public term_visitor {
    ostream& out;

    virtual void visit(variable* t) override {
        out << *(t->atom);
    }

    virtual void visit(compound* t) override {
        if (::ispunct((*(t->atom))[0]) && t->args.size() == 2) {
            t->args[0]->accept(this);
            cout << " " << *(t->atom) << " ";
            t->args[1]->accept(this);
        } else {
            out << *(t->atom);
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

    void operator() (term* t) {
        t->accept(this);
    }
};

ostream& operator<< (ostream& out, term* exp) {
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
    out << endl;
    for (auto j = p.goals.cbegin(); j != p.goals.cend(); ++j) {
        out << *j << endl;
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

using var_t = map<atom_t, variable*>::const_iterator;

struct inherited_attributes {
    program& prog;

    map<atom_t, variable*, atom_less> variables; 
    set<variable*> repeated;
    set<variable*> repeated_in_goal;

    inherited_attributes(inherited_attributes const&) = delete;
    inherited_attributes& operator= (inherited_attributes const&) = delete;

    atom_t get_atom(string const& atom) {
        atom_t const n = prog.atoms.find(atom);
        if (n == prog.atoms.end()) {
            return prog.atoms.insert(atom).first;
        } else {
            return n;
        }
    }

    inherited_attributes(program& p) : prog(p) {}
};

//----------------------------------------------------------------------------
// Grammar
//
// Function objects get passed the results of the sub-parsers, along with the 
// inherited attribute. As the parser-combinators are templated, the inherited
// attribute is whatever type is passed as the final argument to the parser.

struct return_variable {
    return_variable() {}
    void operator() (variable** res, string const& atom, inherited_attributes* st) const {
        atom_t const n = st->get_atom(atom);
        var_t i = st->variables.find(n);
        if (i == st->variables.end()) {
            variable* const var = st->prog.new_variable(n);
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
    void operator() (vector<term*>* res, term* t1, inherited_attributes*) const {
        res->push_back(t1);
    }
} const return_args;

struct return_struct {
    return_struct() {}
    void operator() (compound** res, string const& atom, vector<term*>& args, inherited_attributes* st) const {
        *res = st->prog.new_compound(st->get_atom(atom), args);
    }
} const return_struct;

struct return_term {
    return_term() {}
    void operator() (term** res, int n, variable* v, compound *s, inherited_attributes* st) const {
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
    void operator() (term** res, term* t1, pair<string, term*> const& t2, inherited_attributes* st) const {
        if (!t2.first.empty()) {
            atom_t o = st->get_atom(t2.first);
            vector<term*> a {t1, t2.second};
            *res = st->prog.new_compound(o, a);
        } else {
            *res = t1;
        }
    }
} const return_op_exp_exp;

struct return_op_var_exp {
    return_op_var_exp() {}
    void operator() (compound** res, variable* t1, string const& oper, term* t2, inherited_attributes* st) const {
        atom_t o = st->get_atom(oper);
        vector<term*> a {t1, t2};
        *res = st->prog.new_compound(o, a);
    }
} const return_op_var_exp;

struct return_oper_term {
    return_oper_term() {}
    void operator() (
        pair<string, term*>* res,
        string const& oper,
        term* term,
        inherited_attributes* st
    ) const {
        *res = make_pair(oper, term);
    }
} const return_oper_term;    

struct return_op_stc_exp {
    return_op_stc_exp() {}
    void operator() (compound** res, compound* t1, pair<string, term*> const& t2, inherited_attributes* st) const {
        if (!t2.first.empty()) {
            atom_t o = st->get_atom(t2.first);
            vector<term*> a {t1, t2.second};
            *res = st->prog.new_compound(o, a);
        } else {
            *res = t1;
        }
    }
} const return_op_stc_exp;
 
struct return_head {
    return_head() {}
    void operator() (compound** res, compound* str, inherited_attributes* st) const {
        *res = str;
        st->repeated_in_goal = st->repeated;
    }
} const return_head;

struct return_goal {
    return_goal() {}
    void operator() (vector<compound*>* res, compound* impl, inherited_attributes*) const {
        res->push_back(impl);
    }
} const return_goal;

struct return_clause {
    return_clause() {}
    void operator() (
        void* res,
        compound* head,
        vector<compound*>& impl,
        inherited_attributes* st
    ) const {
        st->prog.db.emplace(head->atom, st->prog.new_clause(head, impl, st->repeated_in_goal));
        st->variables.clear();
        st->repeated.clear();
        st->repeated_in_goal.clear();
    }
} const return_clause;

struct return_goals {
    return_goals() {}
    void operator() (program* res, vector<compound*>& impl, inherited_attributes* st) const {
        vector<term*> vars;
        for (auto v : st->variables) {
            vars.push_back(v.second);
        }

        atom_t n = st->get_atom("goal");
        st->prog.goals.emplace_back(st->prog.new_clause(
            st->prog.new_compound(n, vars), impl, set<variable*> {}));
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
pshand<compound*> recursive_struct(pshand<term*> const t) {
    return define("struct", all(return_struct,
        atom,
        option(discard(open_tok) && sep_by(all(return_args, t), sep_tok) && discard(close_tok))
    ));
}

// parse a term that is either a variable or an atom/struct
pshand<term*> recursive_term(pshand<term*> const t) {
    return define("term", any(return_term,
        variable,
        recursive_struct(t))
    );
}

// parse a term, followed by optional operator and term.
pshand<term*> recursive_oper(pshand<term*> const t) {
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

