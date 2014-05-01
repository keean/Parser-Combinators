#include <iostream>
#include <vector>
#include "templateio.hpp"
#include "parser_simple.hpp"

using namespace std;

//----------------------------------------------------------------------------
//
struct stats {
    int num;
    int sum;
    int sum2;

    stats(int num, int sum, int sum2) : num(num), sum(sum), sum2(sum2) {}
};

ostream& operator<< (ostream &out, stats const& s) {
    double const num = static_cast<double>(s.num);
    double const avg = static_cast<double>(s.sum) / num;
    double const var = static_cast<double>(s.sum2) / num - avg;
    return cout << "{num = " << num << ", avg = " << avg << ", var = " << var << "}";
}


struct csv_parser : private fparse {

    csv_parser(fstream &in) : fparse(in) {}

    bool parse_csv(vector<stats>& s) {
        do {
            if (accept(is_char(EOF))) {
                break;
            }
            s.emplace_back(0, 0, 0);
            do {
                space();
                string n;
                if (number(&n)) {
                    int const i = stoi(n);
                    stats &b = s.back();
                    b.num += 1;
                    b.sum += i;
                    b.sum2 += i * i;
                } else {
                    break;
                }
            } while(accept(is_char(',')));
        } while(accept(is_not(is_char(EOF))));
        return s.size() > 0;
    }

    void operator() () {
        vector<stats> s;

        if (parse_csv(s)) {
            cout << "OK\n";
        } else {
            cout << "FAIL\n";
        }
        
        cout << s << "\n";
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
