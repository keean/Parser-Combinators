#include <iostream>
#include <vector>
#include "templateio.hpp"
#include "parser_simple.hpp"
#include "profile.hpp"

using namespace std;

//----------------------------------------------------------------------------

struct csv_parser : private fparse {

    csv_parser(fstream &in) : fparse(in) {}

    bool parse_csv(vector<vector<int>> &result) {
        profile<csv_parser> p;

        do {
            if (accept(is_char(EOF))) {
                break;
            }
            vector<int> tmp;
            do {
                space();
                string n;
                if (number(&n)) {
                    tmp.push_back(stoi(n));
                } else {
                    break;
                }
            } while(accept(is_char(',')));
            result.push_back(move(tmp));
        } while(accept(is_not(is_char(EOF))));
        return result.size() > 0;
    }

    int operator() () {
        vector<vector<int>> a;

        if (parse_csv(a)) {
            cout << "OK\n";
        } else {
            cout << "FAIL\n";
        }
        
        return get_count(); 
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
