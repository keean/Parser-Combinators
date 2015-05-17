//----------------------------------------------------------------------------
// copyright 2012, 2013, 2014 Keean Schupke
// compile with -std=c++11 
// templateio.h

#ifndef TEMPLATEIO_HPP
#define TEMPLATEIO_HPP

#include <iostream>
#include <vector>
#include <tuple>
#include <map>

using namespace std;

//----------------------------------------------------------------------------
// iostream output for tuples.

template <size_t> struct idx {};

template <typename T, size_t I> ostream& print_tuple(ostream& out, T const& t, idx<I>) {
    out << get<tuple_size<T>::value - I>(t) << ", ";
    return print_tuple(out, t, idx<I - 1>());
}

template <typename T> ostream& print_tuple(ostream& out, T const& t, idx<1>) {
    return out << get<tuple_size<T>::value - 1>(t);
}

template <typename... T> ostream& operator<< (ostream& out, tuple<T...> const& t) {
    out << '<';
    print_tuple(out, t, idx<sizeof...(T)>());
    return out << '>';
}

template <typename S, typename T> ostream& operator<< (ostream& out, pair<S, T> const& p) {
    return out << '(' << p.first << ", " << p.second << ')';
}

//----------------------------------------------------------------------------
// iostream output for vectors.

template <typename T> ostream& operator<< (ostream& out, vector<T> const& v) {
    out << "[";
    for (typename vector<T>::const_iterator i = v.begin(); i != v.end(); ++i) {
        cout << *i;
        typename vector<T>::const_iterator j = i;
        if (++j != v.end()) {
            cout << ", ";
        }
    }
    return out << "]";
}

//----------------------------------------------------------------------------
// iostream output for maps.

template <typename S, typename T> ostream& operator<< (ostream& out, map<S, T> const& m) {
    out << "{";
    for (typename map<S, T>::const_iterator i = m.begin(); i != m.end(); ++i) {
        cout << i->first << " = " << i->second;
        typename map<S, T>::const_iterator j = i;
        if (++j != m.end()) {
            cout << ", ";
        }
    }
    return out << "}";
}

template <typename S, typename T> ostream& operator<< (ostream& out, multimap<S, T> const& m) {
    out << "{";
    for (typename multimap<S, T>::const_iterator i = m.begin(); i != m.end(); ++i) {
        cout << i->first << " = " << i->second;
        typename multimap<S, T>::const_iterator j = i;
        if (++j != m.end()) {
            cout << ", ";
        }
    }
    return out << "}";
}

#endif // TEMPLATEIO_HPP
