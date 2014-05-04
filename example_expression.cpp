#include <fstream>
#include <iostream>
#include <vector>
#include "templateio.hpp"
#include "parser_combinators.hpp"
#include "profile.hpp"

using namespace std;

//----------------------------------------------------------------------------
// Example CSV file parser.

enum op {add = 0, sub = 1, mul = 2, div = 3};

struct return_int {
    return_int() {}
    void operator() (int *res, string &num) const {
        *res = stoi(num);
    }
} const return_int;

struct return_op {
    return_op() {}
    void operator() (enum op *res, int choice, string &add, string &sub, string &mul, string &div) const {
        *res = static_cast<enum op>(choice);
    }
} const return_op;

struct state {
    int acc;
    state() {};
};

struct return_left {
    state &s;
    return_left(state &s) : s(s) {}
    void operator() (int *res, int &left) const {
        s.acc = left;
        *res = s.acc;
    }
};

struct return_right {
    state &s;
    return_right(state &s) : s(s) {}
    void operator() (int *res, enum op &opr, int &right) const {
        switch (opr) {
            case op::add:
                s.acc += right;
                break;
            case op::sub:
                s.acc -= right;
                break;
            case op::mul:
                s.acc *= right;
                break;
            case op::div:
                s.acc /= right;
                break;
        }
        *res = s.acc;
    }
};

auto const recognise_number = some(accept(is_digit));
auto const recognise_space = many(accept(is_space));
auto const parse_operand = all(return_int, recognise_number) && discard(recognise_space);
auto const parse_operator = any(return_op, accept(is_char('+')), accept(is_char('-')),
    accept(is_char('*')), accept(is_char('/'))) && discard(recognise_space);

class csv_parser {
    pstream in;

public:
    csv_parser(fstream &fs) : in(fs) {}

    int operator() () {
        state my_state;
        return_left my_left(my_state);
        return_right my_right(my_state);
        auto const parse = all(my_left, parse_operand) && many(all(my_right, parse_operator, parse_operand));
        decltype(parse)::result_type a; 

        bool b;
        {
            profile<csv_parser> p;
            b = parse(in, &a);
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
