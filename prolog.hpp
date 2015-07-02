#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <map>

#include "stream_iterator.hpp"
#include "templateio.hpp"
#include "profile.hpp"

// This is a work around for the "static auto constexpr" initialisation bug 
// in g++ and clang++.
#define static_auto_constexpr(N, T) \
using N ## _type = decltype(T); \
static N ## _type constexpr N = T;

#define static_auto_const(N, T) \
using N ## _type = decltype(T); \
static N ## _type const N = T;

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

template <typename Base> struct logic_parser {

    using atom_t = std::set<std::string>::const_iterator;

    struct atom_less {
        bool operator() (atom_t const& x, atom_t const& y) const {
            return &(*x) < &(*y);
        }
    };

    struct managed : public Base {
        virtual ~managed() {}
    };

    class variable;
    class compound;

    struct term_visitor {
        virtual void visit(variable *t) = 0;
        virtual void visit(compound *t) = 0;
    };

    struct term : public managed {
        virtual void accept(term_visitor* v) = 0;
        virtual ~term() {}
    };

    class program; 

    struct variable : public term {
        atom_t const atom;
        virtual void accept(term_visitor* v) override {
            v->visit(this);
        }
    private:
        friend program;
        variable(atom_t n) : atom(n) {}
    };

    struct compound : public term {
        atom_t const functor;
        vector<term*> const args;
        virtual void accept(term_visitor* v) override {
            v->visit(this);
        }
    private:
        friend program;
        template <typename As> compound(atom_t f, As&& as) : functor(f),
            args(forward<As>(as)) {}
    };


    struct clause : public managed {
        compound* const head;
        vector<compound*> const impl;
        set<variable*> const reps;

    private:
        friend class program;
        template <typename Is, typename Rs>
        clause(compound* h, Is&& is, Rs&& rs) : head(h),
            impl(forward<Is>(is)), reps(forward<Rs>(rs)) {}
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
        class clause* new_clause(compound* h, Is&& is, Rs&& rs) {
            clause* c = new clause(h, forward<Is>(is), forward<Rs>(rs));
            region.emplace_back(c);
            return c;
        }
    };

    //------------------------------------------------------------------------
    // Show Abstract Syntax

    class expr_show : public term_visitor {
        ostream& out;

        virtual void visit(variable* t) override {
            out << *(t->atom);
        }

        virtual void visit(compound* t) override {
            if (::ispunct((*(t->functor))[0]) && t->args.size() == 2) {
                t->args[0]->accept(this);
                cout << " " << *(t->functor) << " ";
                t->args[1]->accept(this);
            } else {
                out << *(t->functor);
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

    friend  ostream& operator<< (ostream& out, term* exp) {
        expr_show eshow(out);
        eshow(exp);
        return out;
    }

    friend ostream& operator<< (ostream& out, clause* cls) {
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

    friend ostream& operator<< (ostream& out, program const& p) {
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
    }

    //------------------------------------------------------------------------
    // Parser State
    //
    // Making this uncopyable prevents backtracking being used, as the
    // 'attempt' combinator will try and make a copy of the inherited
    // attribute for backtracking. This can be seen as a constraint on the
    // parser (to ensure performance) or a safety measure to make sure we deal
    // with copying if we want backtracking.

    using var_t = typename map<atom_t, variable*>::const_iterator;

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

    //------------------------------------------------------------------------
    // Grammar
    //
    // Function objects get passed the results of the sub-parsers, along with
    // the inherited attribute. As the parser-combinators are templated, the
    // inherited attribute is whatever type is passed as the final argument to
    // the parser.

    static struct return_variable_t {
        constexpr return_variable_t() {}
        void operator() (
            variable** res,
            string const& atom,
            inherited_attributes* st
        ) const {
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
    } constexpr return_variable {};

    static struct return_args_t {
        constexpr return_args_t() {}
        void operator() (
            vector<term*>* res,
            term* t1,
            inherited_attributes*
        ) const {
            res->push_back(t1);
        }
    } constexpr return_args {};

    static struct return_struct_t {
        constexpr return_struct_t() {}
        void operator() (
            compound** res,
            string const& atom,
            vector<term*>& args,
            inherited_attributes* st
        ) const {
            *res = st->prog.new_compound(st->get_atom(atom), args);
        }
    } constexpr return_struct {};

    static struct return_term_t {
        constexpr return_term_t() {}
        void operator() (
            term** res,
            int n,
            variable* v,
            compound *s,
            inherited_attributes* st
        ) const {
            switch (n) {
                case 0:
                    *res = v;
                    break;
                case 1:
                    *res = s;
                    break;
            }
        }
    } constexpr return_term {};

    static struct return_op_exp_exp_t {
        constexpr return_op_exp_exp_t() {}
        void operator() (
            term** res,
            term* t1,
            pair<string, term*> const& t2,
            inherited_attributes* st
        ) const {
            if (!t2.first.empty()) {
                atom_t o = st->get_atom(t2.first);
                vector<term*> a {t1, t2.second};
                *res = st->prog.new_compound(o, a);
            } else {
                *res = t1;
            }
        }
    } constexpr return_op_exp_exp {};

    static struct return_op_var_exp_t {
        constexpr return_op_var_exp_t() {}
        void operator() (
            compound** res,
            variable* t1,
            string const& oper, term* t2,
            inherited_attributes* st
        ) const {
            atom_t o = st->get_atom(oper);
            vector<term*> a {t1, t2};
            *res = st->prog.new_compound(o, a);
        }
    } constexpr return_op_var_exp {};

    static struct return_oper_term_t {
        constexpr return_oper_term_t() {}
        void operator() (
            pair<string, term*>* res,
            string const& oper,
            term* term,
            inherited_attributes* st
        ) const {
            *res = make_pair(oper, term);
        }
    } constexpr return_oper_term {};    

    static struct return_op_stc_exp_t {
        constexpr return_op_stc_exp_t() {}
        void operator() (
            compound** res,
            compound* t1,
            pair<string, term*> const& t2,
            inherited_attributes* st
        ) const {
            if (!t2.first.empty()) {
                atom_t o = st->get_atom(t2.first);
                vector<term*> a {t1, t2.second};
                *res = st->prog.new_compound(o, a);
            } else {
                *res = t1;
            }
        }
    } constexpr return_op_stc_exp {};
 
    static struct return_head_t {
        constexpr return_head_t() {}
        void operator() (
            compound** res,
            compound* str,  
            inherited_attributes* st
        ) const {
            *res = str;
            st->repeated_in_goal = st->repeated;
        }
    } constexpr return_head {};

    static struct return_goal_t {
        constexpr return_goal_t() {}
        void operator() (
            vector<compound*>* res,
            compound* impl,
            inherited_attributes*
        ) const {
            res->push_back(impl);
        }
    } constexpr return_goal {};

    static struct return_clause_t {
        constexpr return_clause_t() {}
        void operator() (
            void* res,
            compound* head,
            vector<compound*>& impl,
            inherited_attributes* st
        ) const {
            st->prog.db.emplace(head->functor,
                st->prog.new_clause(head, impl, st->repeated_in_goal));
            st->variables.clear();
            st->repeated.clear();
            st->repeated_in_goal.clear();
        }
    } constexpr return_clause {};

    static struct return_goals_t {
        constexpr return_goals_t() {}
        void operator() (program* res,
            vector<compound*>& impl,
            inherited_attributes* st
        ) const {
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
    } constexpr return_goals {};

    //------------------------------------------------------------------------
    // Parser
    //
    // The parsers and user grammar are all stateless, so can be const objects
    // and still thread safe. The final composed parser is a regular procedure
    // , all state is either in the state object passed in, or in the returned
    // values.

    static_auto_constexpr(atom_tok, tokenise(accept(is_lower)
        && many(accept(is_alnum || is_char('_')))));
    static_auto_constexpr(var_tok, tokenise(accept(is_upper || is_char('_'))
        && many(accept(is_alnum || is_char('_')))));
    static_auto_constexpr(open_tok, tokenise(accept(is_char('('))));
    static_auto_constexpr(close_tok, tokenise(accept(is_char(')'))));
    static_auto_constexpr(sep_tok, tokenise(accept(is_char(','))));
    static_auto_constexpr(end_tok, tokenise(accept(is_char('.'))));
    static_auto_constexpr(impl_tok, tokenise(accept_str(":-")));
    static_auto_constexpr(oper_tok, tokenise(some(accept(
        is_punct - (is_char('_')  || is_char('(')  || is_char(')')
        || is_char(',')))) - "." - ":-"));
    static_auto_constexpr(comment_tok, tokenise(accept(is_char('#'))
        && many(accept(is_print)) && accept(is_eol)));

    // the "definitions" help clean up the EBNF output in error reports
    static_auto_constexpr(var, define("variable",
        all(return_variable, var_tok)));
    static_auto_constexpr(atom, define("atom", atom_tok));
    static_auto_constexpr(oper, define("operator", oper_tok));

    template <typename T> using pshand =
        pstream_handle<T, inherited_attributes>;

    // higher order parsers.

    // parse atom and list of arguments.
    static pshand<compound*> recursive_struct(pshand<term*> const& t) {
        return define("struct", all(return_struct, atom,
            option(discard(open_tok) && sep_by(all(return_args, t),
            sep_tok) && discard(close_tok))));
    }

    // parse a term that is either a variable or an atom/struct
    static pshand<term*> recursive_term(pshand<term*> const& t) {
        return define("term", any(return_term, var, recursive_struct(t)));
    }

    // parse a term, followed by optional operator and term.
    static pshand<term*> recursive_oper(pshand<term*> const& t) {
        return all(return_op_exp_exp, recursive_term(t),
            option(all(return_oper_term, attempt(oper), t)));
    }

    //------------------------------------------------------------------------

    template <typename Range>
    static int parse(Range const& r, program& prog) {
        auto const op = fix("op-list", recursive_oper);
        auto const structure = define("op-struct", all(return_op_var_exp, var,
            oper, op) || all(return_op_stc_exp, recursive_struct(op),
            option(all(return_oper_term, attempt(oper), op))));
        auto const comment = define("comment", discard(comment_tok));
        auto const goals = define("goals", discard(impl_tok)
            && sep_by(all(return_goal, structure), discard(sep_tok)));
        auto const query  = define("query", all(return_goals, goals)
            && discard(end_tok));
        auto const clause = define("clause", all(return_clause,
            all(return_head, structure), option(goals) && discard(end_tok)));
        auto const parser = first_token && strict("unexpected character",
            some(clause || query || comment));

        typename Range::iterator i = r.first;
        inherited_attributes st(prog);
        parser(i, r, &prog, &st);
        return i - r.first;
    }
};

template <typename T> constexpr typename logic_parser<T>::atom_tok_type logic_parser<T>::atom_tok;
template <typename T> constexpr typename logic_parser<T>::var_tok_type logic_parser<T>::var_tok;
template <typename T> constexpr typename logic_parser<T>::open_tok_type logic_parser<T>::open_tok;
template <typename T> constexpr typename logic_parser<T>::close_tok_type logic_parser<T>::close_tok;
template <typename T> constexpr typename logic_parser<T>::sep_tok_type logic_parser<T>::sep_tok;
template <typename T> constexpr typename logic_parser<T>::end_tok_type logic_parser<T>::end_tok;
template <typename T> constexpr typename logic_parser<T>::impl_tok_type logic_parser<T>::impl_tok;
template <typename T> constexpr typename logic_parser<T>::oper_tok_type logic_parser<T>::oper_tok;
template <typename T> constexpr typename logic_parser<T>::comment_tok_type logic_parser<T>::comment_tok;
template <typename T> constexpr typename logic_parser<T>::var_type logic_parser<T>::var;
template <typename T> constexpr typename logic_parser<T>::atom_type logic_parser<T>::atom;
template <typename T> constexpr typename logic_parser<T>::oper_type logic_parser<T>::oper;

