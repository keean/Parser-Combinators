#include <iostream>
#include <cstdlib>
#include <cstring>

using namespace std;

int expr(int depth, int max_depth) {
    if (++depth > max_depth) {
        int const v = rand() % 10 + 1;
        cout << v;
        return v;
    }

    cout << "(";
    int const u = expr(depth, max_depth);
    switch (rand() % 4) {
        case 0: {
            cout << " + ";
            int const v = expr(depth, max_depth);
            cout << ")";
            return u + v;
        }

        case 1: {
            cout << " - ";
            int const v = expr(depth, max_depth);
            cout << ")";
            return u - v;
        }

        case 2: {
            cout << " * (";
            int const v = expr(depth, max_depth);
            if (v == 0) {
                cout << " + 1))";
                return u * (v + 1);
            } else {
                cout << " + 0))";
                return u * v;
            }
            cout << ")";
            return u * v;
        }

        default: {
            cout << " / (";
            int const v = expr(depth, max_depth);
            if (v == 0) {
                cout << " + 1))";
                return u / (v + 1);
            } else {
                cout << " + 0))";
                return u / v;
            }
        }
    }
}

int main() {
    int max_depth = 12;
    int depth = 0;

    int const v = expr(depth, max_depth);
    cout << endl;
    cerr << " = " << v << endl;
}
