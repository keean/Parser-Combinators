#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

#include "templateio.hpp"
#include "parser_combinators.hpp"
#include "profile.hpp"

using namespace std;

//----------------------------------------------------------------------------
// Example Expression Evaluating File Parser.

struct return_int {
    return_int() {}
    void operator() (int *res, string &num) const {
        *res = stoi(num);
    }
} const return_int;

struct return_add {
    return_add() {}
    void operator() (int *res, int left, string&, int right) const {
        *res = left + right;
    }
} const return_add;

struct return_sub {
    return_sub() {}
    void operator() (int *res, int left, string&, int right) const {
        *res = left - right;
    }
} const return_sub;

struct return_mul {
    return_mul() {}
    void operator() (int *res, int left, string&, int right) const {
        *res = left * right;
    }
} const return_mul;

struct return_div {
    return_div() {}
    void operator() (int *res, int left, string&, int right) const {
        *res = left / right;
    }
} const return_div;

auto const number_tok = tokenise(some(accept(is_digit)));
auto const start_tok = tokenise(accept(is_char('(')));
auto const end_tok = tokenise(accept(is_char(')')));
auto const add_tok = tokenise(accept(is_char('+')));
auto const sub_tok = tokenise(accept(is_char('-')));
auto const mul_tok = tokenise(accept(is_char('*')));
auto const div_tok = tokenise(accept(is_char('/')));

parser_handle<int> const additive_expr(parser_handle<int> e) {
    return log("+", attempt(all(return_add, e, add_tok, e)))
        || log("-", all(return_sub, e, sub_tok, e));
}

parser_handle<int> const multiplicative_expr(parser_handle<int> e) {
    return log("*", attempt(all(return_mul, e, mul_tok, e)))
        || log("/", all(return_div, e, div_tok, e));
}

parser_handle<int> const expression = attempt(
        discard(start_tok)
        && (attempt(additive_expr(reference("expr", expression)))
            || multiplicative_expr(reference("expr", expression)))
        && discard(end_tok))
    || all(return_int, number_tok);

class csv_parser {
    pstream in;

public:
    csv_parser(fstream &fs) : in(fs) {}

    int operator() () {
        auto const parser = strict("invalid expression", expression);

        decltype(parser)::result_type a {}; 

        bool b;
        try {
            profile<csv_parser> p;
            b = parser(in, &a);
        } catch (parse_error& e) {
            stringstream err;
            err << e;
            throw runtime_error(err.str());
        }

        if (b) {
            cout << "OK\n";
        } else {
            cout << "FAIL\n";
        }

        cout << a << "\n";
        
        return in.get_pos();
    }
};

//----------------------------------------------------------------------------

int main(int const argc, char const *argv[]) {
    if (argc < 1) {
        cerr << "no input files\n";
    } else {
        for (int i = 1; i < argc; ++i) {
            fstream in(argv[i], ios_base::in);
            cout << argv[i] << "\n";

            if (in.is_open()) {
                csv_parser csv(in);
                profile<csv_parser>::reset();
                int const chars_read = csv();
                double const mb_per_s = static_cast<double>(chars_read) / static_cast<double>(profile<csv_parser>::report());
                cout << "parsed: " << mb_per_s << "MB/s\n";
            }
        }
    }
}
