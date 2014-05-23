#include <fstream>
#include <iostream>
#include <vector>
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

auto const recognise_space = many(accept(is_space));
auto const recognise_number = discard(recognise_space) && some(accept(is_digit));
auto const recognise_start = discard(recognise_space) && accept(is_char('('));
auto const recognise_end = discard(recognise_space) && accept(is_char(')'));

parser_handle<int> const additive_expr(parser_handle<int> const e) {
    return log("+", all(return_add, e, discard(recognise_space) && accept(is_char('+')), e))
        || log("-", all(return_sub, e, discard(recognise_space) && accept(is_char('-')), e));
}

parser_handle<int> const multiplicative_expr(parser_handle<int> const e) {
    return log("*", all(return_mul, e, discard(recognise_space) && accept(is_char('*')), e))
        || log("/", all(return_div, e, discard(recognise_space) && accept(is_char('/')), e));
}

parser_handle<int> const expression = (
        discard(recognise_start) && (
            additive_expr(reference(expression)) || multiplicative_expr(reference(expression))
        ) && discard(recognise_end)
    ) || all(return_int, recognise_number);

class csv_parser {
    pstream in;

public:
    csv_parser(fstream &fs) : in(fs) {}

    int operator() () {
        auto const parser = expression;

        decltype(parser)::result_type a {}; 

        bool b;
        {
            profile<csv_parser> p;
            b = parser(in, &a);
        }

        if (b) {
            cout << "OK\n";
        } else {
            cout << "FAIL\n";
        }

        cout << a << "\n";
        
        return in.get_count();
    }
};

//----------------------------------------------------------------------------

int main(int const argc, char const *argv[]) {
    if (argc < 1) {
        cerr << "no input files\n";
    } else {
        for (int i = 1; i < argc; ++i) {
            try {
                fstream in(argv[i], ios_base::in);
                cout << argv[i] << "\n";

                if (in.is_open()) {
                    csv_parser csv(in);
                    profile<csv_parser>::reset();
                    int const chars_read = csv();
                    double const mb_per_s = static_cast<double>(chars_read) / static_cast<double>(profile<csv_parser>::report());
                    cout << "parsed: " << mb_per_s << "MB/s\n";
                }
            } catch (parse_error& e) {
                cerr << argv[i] << ": " << e.what()
                    << " " << e.exp
                    << " found ";
                if (is_print(e.sym)) {
                    cerr << "'" << static_cast<char>(e.sym) << "'";
                } else {
                    cerr << "0x" << hex << e.sym;
                }
                cerr << " at line " << e.row
                    << ", column " << e.col << "\n";
                return 2;
            }
        }
    }
}
