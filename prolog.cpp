#include"prolog.hpp" 

using namespace std;

struct expression_parser;

template <typename Range>
int parse(Range const &r, program& prog) {
    decltype(parser)::result_type a {}; 
    typename Range::iterator i = r.first;
    inherited_attributes st(prog.names);

    profile<expression_parser> p;

    if (parser(i, r, &prog.db, &st)) {
        cout << "OK" << endl;
    } else {
        cout << "FAIL" << endl;
    }

    return i - r.first;
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
            program prog;
            int const chars_read = parse(in, prog);
            cout << prog << endl;
            //double const mb_per_s = static_cast<double>(chars_read) / static_cast<double>(profile<expression_parser>::report());
            //cout << "parsed: " << mb_per_s << "MB/s" << endl;
        }
    }
}
