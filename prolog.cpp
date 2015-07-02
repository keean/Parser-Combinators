#include"prolog.hpp" 

using namespace std;

struct base {};
using lp = logic_parser<base>;

struct expression_parser;

template <typename Range>
int parse(Range const &r, lp::program& prog) {
    profile<expression_parser> p;
    return lp::parse(r, prog);
}

//----------------------------------------------------------------------------
// The stream_range allows file iterators to be used like random_iterators
// and abstracts the difference between C++ stdlib streams and file_vectors.


int main(int const argc, char const *argv[]) {
    if (argc < 1) {
        cerr << "no input files" << endl;
    } else {
        for (int i = 1; i < argc; ++i) {
            profile<expression_parser>::reset();
            stream_range in(argv[i]);
            cout << argv[i] << endl;
            lp::program prog;
            int const chars_read = parse(in, prog);
            cout << prog;
            double const mb_per_s = static_cast<double>(chars_read) / static_cast<double>(profile<expression_parser>::report());
            cout << "parsed: " << mb_per_s << "MB/s" << endl;
        }
    }
}
