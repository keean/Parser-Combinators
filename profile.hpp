//----------------------------------------------------------------------------
// copyright 2012, 2013, 2014 Keean Schupke
// compile with c++ -std=c++11 
// profile.h

#ifndef PROFILE_HPP
#define PROFILE_HPP

#include <ctime>

extern "C" {
    #include <sys/resource.h>
}

inline uint64_t rtime() {
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    return 1000000 * static_cast<uint64_t>(rusage.ru_utime.tv_sec)
        + static_cast<uint64_t>(rusage.ru_utime.tv_usec);
}

template <typename T> class profile {
    static uint64_t t;
    static uint64_t s;

public:
    profile() {
        start();
    }

    ~profile() {
        finish();
    }

    static void start() {
        s = rtime();
    }

    static void finish() {
        uint64_t const u = rtime();
        t += u - s;
        s = u;
    }

    static void reset() {
        t = 1;
    }

    static uint64_t report() {
        return t;
    }
};

template<typename T> uint64_t profile<T>::t {1};
template<typename T> uint64_t profile<T>::s;

#endif // PROFILE_HPP
