#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>

#include "templateio.hpp"
#include "parser_combinators.hpp"
#include "profile.hpp"

using namespace std;

//----------------------------------------------------------------------------
// Example CSV file parser.

struct parse_int {
    parse_int() {}
    void operator() (vector<int> *ts, string const& num) const {
        ts->push_back(stoi(num));
    }
} const parse_int;

struct parse_line {
    parse_line() {}
    void operator() (vector<vector<int>> *ts, vector<int> &line) const {
        ts->push_back(move(line)); // move modifies 'line' so don't make it const
    }
} const parse_line;

auto const number_tok = tokenise(some(accept(is_digit)));
auto const separator_tok = tokenise(accept(is_char(',')));

auto const parse_csv = strict("error parsing csv",
    next_token && some(all(parse_line, sep_by(all(parse_int, number_tok), separator_tok)))
);

class csv_parser {
    pstream in;

public:
    csv_parser(fstream &fs) : in(fs) {}

    int operator() () {
        decltype(parse_csv)::result_type a; 

        bool b;
        try {
            profile<csv_parser> p;
            b = parse_csv(in, &a);
        } catch (parse_error& e) {
            stringstream err;
            err << e.what();
            throw runtime_error(err.str());
        }

        if (b) {
            cout << "OK\n";
        } else {
            cout << "FAIL\n";
        }

        int sum = 0;
        for (int i = 0; i < a.size(); ++i) {
            for (int j = 0; j < a[i].size(); j++) {
               sum += a[i][j];
            }
        }
        sum /= a.size();
        cerr << sum << endl;
        
        return in.get_location().pos;
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
