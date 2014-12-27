#include <iostream>
//#include <cstdlib>
//#include <cstring>

using namespace std;

int main() {
    int s = 0, s2 = 0, n = 0;
    for (int i = 0; i < 10000; ++i) {
        int t = 0;
        for (int j = 0; j < 1000; ++ j) {
            int const v = rand() % 10 + 1;
            cout << v << ", ";
            t += v;
        }
        int const v = rand() % 10 + 1;
        cout << v << endl;
        t += v;
        s += t;
        ++n;

    }

    cerr << " = " << (s / n) << endl;
}
