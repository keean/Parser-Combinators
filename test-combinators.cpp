#include <iostream>
#include <vector>
#include "templateio.h"
#include "parser-combinators.h"

using namespace std;

//----------------------------------------------------------------------------
// Example CSV file parser, calculates average and variance for each line in
// an integer CSV file:

// result type
struct stats {
    int n;
    int sum;
    int sum2;
};

ostream& operator<< (ostream &out, stats const& s) {
    double const num = static_cast<double>(s.n);
    double const avg = static_cast<double>(s.sum) / num;
    double const var = static_cast<double>(s.sum2) / num - avg;
    return cout << "{num = " << num << ", avg = " << avg << ", var = " << var << "}";
}

// function to fold stats into a vector
template <typename T> struct vec {
    vec() {};
    void operator() (vector<T> *ts, T &t) const {
        ts->push_back(t);
    }
};

auto const vec_stats = vec<stats>() ;

// functon to fold CSV lines into sum and sum-of-squares.
struct acc {
    acc() {};
    void operator() (stats *s, string &t) const {
        int const i = stoi(t);
        s->n += 1;
        s->sum += i;
        s->sum2 += i * i;
    }
} const acc;

auto const recognise_number = many(accept(is_digit));
auto const recognise_space = many(accept(is_space));
auto const recognise_separator = option(accept(is_char(',')) && recognise_space);
auto const parse_csv = fold(vec_stats, fold(acc, recognise_number && discard(recognise_separator)) && discard(option(recognise_space)));

class csv_parser {
    fparse in;

public:
    csv_parser(fstream &fs) : in(fs) {}

    void operator() () {
        decltype(parse_csv)::result_type a; 

        if (parse_csv(in, &a)) {
            cout << "OK\n";
        } else {
            cout << "FAIL\n";
        }
        
        cout << a << "\n";
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
                    csv();
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
