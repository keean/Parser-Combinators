#include <fstream>
#include <iostream>
#include <vector>
#include "templateio.h"
#include "parser-combinators.h"

using namespace std;

//----------------------------------------------------------------------------
// Example CSV file parser.

struct parse_int {
    parse_int() {};
    void operator() (vector<int> *ts, string &num, string &sep) const {
        ts->push_back(stoi(num));
    }
} const parse_int;

struct parse_line {
    parse_line() {}
    void operator() (vector<vector<int>> *ts, vector<int> &line, string &line_sep) const {
        ts->push_back(move(line));
    }
} const parse_line;

auto const recognise_number = some(accept(is_digit));
auto const recognise_space = some(accept(is_space));
auto const recognise_separator = option(accept(is_char(',')) && discard(option(recognise_space)));
auto const parse_csv = some(fmap(parse_line, some(fmap(parse_int, recognise_number, recognise_separator)), option(recognise_space)));

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
