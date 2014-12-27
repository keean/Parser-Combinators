#include <fstream>
#include <iostream>
#include <vector>
#include "templateio.hpp"
#include "parser_simple.hpp"
#include "profile.hpp"

using namespace std;

//----------------------------------------------------------------------------

struct csv_parser : private parser {

    csv_parser(istream &in) : parser(in) {}

    bool separator(string *s = nullptr) {
        accept(is_char(','), s) && space();
        return true;
    }

    bool parse_int(vector<int> &ts) {
        string n;
        if (number(&n) && separator()) {
            ts.push_back(stoi(n));
            return true;
        }
        return false;
    }

    bool many_ints(vector<int> &ts) {
        if (parse_int(ts)) {
            while (parse_int(ts));
            return true;
        }
        return false;
    }

    bool parse_line(vector<vector<int>> &ts) {
        vector<int> t {};
        if (many_ints(t)) {
            space();
            ts.push_back(move(t));
            return true;
        }
        return false;
    }

    bool parse_csv(vector<vector<int>> &result) {
        profile<csv_parser> p;

        if (parse_line(result)) {
            while (parse_line(result));
            return true;
        }
        return false;
    }

    int operator() () {
        vector<vector<int>> a;

        if (parse_csv(a)) {
            cout << "OK" << endl;
        } else {
            cout << "FAIL" << endl;
        }

        int sum = 0;
        for (int i = 0; i < a.size(); ++i) {
            for (int j = 0; j < a[i].size(); ++j) {
                sum += a[i][j];
            }
        }
        sum /= a.size();
        cerr << sum << endl;
        
        return get_count(); 
    }
};

//----------------------------------------------------------------------------

int main(int const argc, char const *argv[]) {
    if (argc < 1) {
        cerr << "no input files" << endl;
    } else {
        for (int i = 1; i < argc; ++i) {
            try {
                fstream in(argv[i], ios_base::in);
                cout << argv[i] << endl;

                if (in.is_open()) {
                    csv_parser csv(in);
                    profile<csv_parser>::reset();
                    int const chars_read = csv();
                    double const mb_per_s = static_cast<double>(chars_read) / static_cast<double>(profile<csv_parser>::report());
                    cout << "parsed: " << mb_per_s << "MB/s" << endl;
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
                    << ", column " << e.col << endl;
                return 2;
            }
        }
    }
}
